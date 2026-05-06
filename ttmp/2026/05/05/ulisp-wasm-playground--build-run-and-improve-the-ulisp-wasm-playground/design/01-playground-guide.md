---
Title: 'uLisp WASM Playground: Architecture, Build, and Improvement Guide'
Ticket: ulisp-wasm-playground
Status: active
Topics:
    - wasm
    - lisp
    - web
    - playground
DocType: design
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ulisp-wasm/build.ts
      Note: Build script for all targets
    - Path: ulisp-wasm/c99/ulisp.c
      Note: C interpreter source (WASM core)
    - Path: ulisp-wasm/editor/index.ts
      Note: CodeMirror 6 editor setup
    - Path: ulisp-wasm/node/common.js
    - Path: ulisp-wasm/site/lib/create-lisp.ts
      Note: Worker bridge and request/response protocol
    - Path: ulisp-wasm/site/lib/ulisp-worker.ts
      Note: Worker thread script
    - Path: ulisp-wasm/site/pages/index.tsx
      Note: Main React page component
    - Path: ulisp-wasm/vite.config.ts
    - Path: ulisp-wasm/web/web.ts
      Note: Browser WASM wrapper with host callbacks
ExternalSources: []
Summary: Comprehensive intern guide to understanding, building, testing, and improving the uLisp WASM playground.
LastUpdated: 2026-05-05T20:30:00-04:00
WhatFor: Onboarding reference for anyone working on the playground
WhenToUse: Read this before making any changes to the ulisp-wasm playground
---


# uLisp WASM Playground: Architecture, Build, and Improvement Guide

## What You Are Looking At

The uLisp WASM playground is a web application that runs a full Lisp interpreter inside your browser. You type Lisp code into a code editor on the left side of the screen, press Run (or Ctrl+Enter), and the code executes. Output appears in a console panel on the right. No server is involved in the execution — the Lisp interpreter is compiled to WebAssembly and runs entirely in your browser.

This guide explains every layer of the system, from the C source code to the React components, so you can understand what each piece does and how to change it.

---

## 1. The Big Picture: Data Flow

When you press Run, this is what happens:

```
[CodeMirror Editor]
        |
        | user code as string
        v
[React Page Component]  ---postMessage--->  [Web Worker]
        |                                       |
        |                                       |  createLisp()
        |                                       v
        |                                  [Emscripten Module]
        |                                       |
        |                                       |  WASM executes C code
        |                                       v
        |                                  [c99/ulisp.c interpreter]
        |                                       |
        |  <---postMessage---                   |  print() callbacks
        |                                       |
        v                                       v
[Console Panel]                           [Step counter updates]
```

The key architectural decision is that the WASM interpreter runs inside a **Web Worker**, not the main browser thread. This matters because Lisp evaluation can take a long time — a recursive Fibonacci call with a large argument could block the browser for seconds. By running in a Worker, the UI stays responsive. You can even click Stop to terminate the Worker mid-execution.

The communication between the main thread (React UI) and the Worker uses the browser's `postMessage` API with a request/response protocol built on top of it.

---

## 2. The Layers, From Bottom to Top

### Layer 1: C Source (`c99/ulisp.c`)

This is the heart of the system. A 9,000-line C file containing the complete uLisp interpreter, ported from the ESP32 Arduino version to pure C99. It has no dependencies beyond the C standard library and POSIX.

The interpreter is structured as:

- **Reader** — parses text input into Lisp object trees (lists, symbols, numbers)
- **Evaluator** — walks the object tree and executes functions
- **Printer** — converts Lisp objects back to text
- **Garbage collector** — mark-and-sweep, reclaims unused objects
- **REPL** — read-eval-print loop (though in WASM mode this is driven by the host)

For the WASM build, the C code exposes three functions via Emscripten's `ccall` interface:

```c
// Called once at startup to allocate workspace
void setup();

// Called to evaluate a string of Lisp code
void evaluate(char* code);

// Called to stop a running evaluation loop
void stop_loop();

// Called to print the version string
void print_version();
```

The C code also calls *back* into JavaScript via `EM_ASYNC_JS` macros (Emscripten's async host binding). These callbacks handle things the C code cannot do on its own: printing output, reading input, and delaying execution. The callbacks are:

```javascript
// Host callbacks called FROM the WASM interpreter:
ulisp.writeLine(data)       // Print a line of output
ulisp.writeLineError(data)  // Print an error
ulisp.readLine()            // Wait for user input (async)
ulisp.delay(duration)       // Sleep for N milliseconds (async)
ulisp.wait_for_tick()       // Yield control back to JS event loop
ulisp.escape()              // Check if user pressed Escape/Stop
ulisp.call(command, args)   // Hardware emulation (analogRead, etc.)
ulisp.readByte(streamType)  // Stream I/O read
ulisp.writeByte(streamType, data) // Stream I/O write
```

The `wait_for_tick()` callback is the mechanism that lets the UI remain responsive during long-running evaluations. The C interpreter calls it periodically (on every AST node evaluation), and the JS host can use it to check for stop requests or update the step counter.

### Layer 2: Emscripten Build (`build.ts`, `docker-compose.yml`)

The C source is compiled to WebAssembly using **Emscripten**, the standard C-to-WASM compiler. The build configuration is in `build.ts`, in the `build:web` case:

```typescript
// Simplified from build.ts
emcc c99/ulisp.c -I c99 -o web/ulisp.js \
  -sASSERTIONS \
  -O2 \
  -fms-extensions \
  -sENVIRONMENT=web \
  -sEXPORT_NAME=createLispWasmModule \
  -sASYNCIFY \              // Enables async host callbacks
  -sMODULARIZE \            // Wraps output as a JS module factory
  -g \
  -sWASM=1 \
  -s "EXPORTED_FUNCTIONS=[ '_setup', '_evaluate', '_free', '_print_version', '_stop_loop' ]" \
  -s "EXPORTED_RUNTIME_METHODS=[ 'ccall', 'cwrap', 'stringToNewUTF8', 'UTF8ToString' ]"
```

Key flags:

| Flag | Why it matters |
|------|---------------|
| `-sASYNCIFY` | Lets C code call async JS functions (like `delay` and `readLine`). Emscripten inserts yield points so the WASM execution can pause and resume. |
| `-sMODULARIZE` | Wraps the output as `createLispWasmModule()` factory function instead of a global `Module` object. This is how the Worker creates its own WASM instance. |
| `-sENVIRONMENT=web` | Optimizes for browser (no Node.js shims). |
| `EXPORTED_FUNCTIONS` | The C functions exposed to JavaScript. |

The build uses Docker because Emscripten is a heavy toolchain. The `docker-compose.yml` mounts the `c99/`, `web/`, `public/`, and `node/` directories and runs `emcc` inside an `emscripten/emsdk:4.0.19` container. You need Docker running to rebuild the WASM from C.

**Output:** Two files — `web/ulisp.js` (82 KB, the JS loader/glue) and `public/ulisp.wasm` (1.1 MB, the compiled binary). The JS file is the module factory; the WASM file is the actual interpreter.


### Layer 3: Web Wrapper (`web/web.ts`, `node/common.js`)

`web/web.ts` is the browser-specific wrapper around the WASM module. It creates the `ulisp` global object (which the C code calls back into) and delegates to `node/common.js` for the core eval logic.

`node/common.js` provides the `lispCreator()` function, which:

1. Creates the WASM module instance via the factory function
2. Calls `Module._setup()` to initialize the Lisp workspace
3. Returns an object with an `eval(code)` method

The `eval()` method works like this:

```javascript
async function evaluate(code) {
  // Wrap user code in (progn ...) so multiple expressions execute
  const wrappedCode = `(progn ${code})`
  
  // Allocate WASM memory for the string
  const ptr = Module.stringToNewUTF8(wrappedCode)
  
  // Call the C evaluate function (async due to Emscripten Asyncify)
  await Module.ccall('evaluate', 'void', ['number'], [ptr], { async: true })
  
  // Free the WASM memory
  Module._free(ptr)
  
  // Collect buffered output
  return printBuffer.end()
}
```

The `printBuffer` is a simple accumulator. Every time the C interpreter calls `ulisp.writeLine()`, the text is appended to the buffer. After evaluation completes, the buffer contents become the eval result.

### Layer 4: Worker Bridge (`site/lib/ulisp-worker.ts`, `site/lib/create-lisp.ts`)

This is where the architecture gets clever. The WASM interpreter runs inside a **Web Worker** — a separate browser thread that cannot access the DOM. The main thread (React) communicates with the Worker via `postMessage`.

**The Worker** (`site/lib/ulisp-worker.ts`):

```
Main thread                    Worker thread
    |                              |
    |  postMessage({code})         |
    |----------------------------->|
    |                              |  createLisp() loads WASM
    |                              |  lisp.eval(code)
    |                              |    ... C interpreter runs ...
    |  <--- {step: 42}            |    (step counter updates)
    |  <--- {print: "5"}          |    (output arrives)
    |  <--- {id, result}          |  eval completes
    |                              |
```

The Worker receives `{id, data: {code, wasmPath}}` messages. For each message, it evaluates the code and posts back `{id, result}` when done. During evaluation, it posts step counts and print output as separate messages.

**The Host** (`site/lib/create-lisp.ts`):

This is the main-thread side. It creates the Worker, manages a request/response map with timeouts, and translates Worker messages into React state updates.

The `createLisp()` function returns a Lisp controller object with three methods:

```typescript
interface LispController {
  eval(code: string): Promise<string>   // Evaluate code, get result
  stop(): void                          // Terminate the Worker
  restart(): Promise<void>              // Kill and recreate the Worker
}
```

When `stop()` is called, it terminates the Worker entirely (via `worker.terminate()`). The next `eval()` call automatically restarts it. This is how the Stop button works — it does not ask the interpreter to stop gracefully; it kills the thread.

The `createWorkerRequest()` helper implements a request/response protocol on top of raw `postMessage`:

```pseudocode
function send(data, timeout):
  id = generateUniqueId()
  requestMap[id] = { resolve, reject, timeoutId }
  worker.postMessage({ id, data })
  return Promise that resolves when response arrives or timeout expires

on worker message:
  if message has { id, result }:
    requestMap[id].resolve(result)
  if message has { step }:
    call step callback (updates React state)
  if message has { print }:
    call print callback (appends to console)
  if message has { action: 'readLine' }:
    show input prompt, wait for user, post response back to worker
```

The bidirectional communication handles four message types:
- **Request/response** — eval calls from host to worker
- **Step updates** — worker to host, for the step counter
- **Print output** — worker to host, for console display
- **Action requests** — worker asks host to do things (readLine, readByte, writeByte)

### Layer 5: React UI (`site/pages/index.tsx`, `site/ui/Layout.tsx`)

The main page is a single React component in `site/pages/index.tsx`. It is a ~250-line component that manages:

**State variables:**
```typescript
const [consoleOut, setConsoleOut] = useState('')     // Console output text
const [currentStep, setStep] = useState(0)            // AST step counter
const [inputPrompt, setInputPrompt] = useState(false)  // Show input dialog?
const [inputValue, setInputValue] = useState('')      // Input field value
const [playerState, setPlayerState] = useState('stopped') // 'running' | 'stopped' | 'cleared'
```

**Layout structure:**
```
+--------------------------------------------------+
| [Logo] uLisp-Wasm Playground  [Run] [Stop] [Share] [GH] |
+------------------------+-------------------------+
| Editor                 | Console                 |
| (CodeMirror 6)         | (pre > code)            |
|                        |                         |
| (defun fib (n) ...)   | 5                       |
| (fib 5)               | Hello from WASM!        |
|                        | Step 198                |
|                        +-------------------------+
|                        | [Input prompt] (when    |
|                        |  readLine is called)    |
+------------------------+-------------------------+
| Shortcut: CTRL+Enter   |                         |
+------------------------+-------------------------+
```

**Key interactions:**

- **Run button** calls `evalRef.current()`, which gets the editor content and calls `lisp.eval(code)`
- **Stop button** calls `lisp.stop()`, which terminates the Worker
- **Share button** base64-encodes the editor content into the URL hash for shareable links
- **Ctrl+Enter** keyboard shortcut also triggers eval (configured in the CodeMirror keymap)

**The print callback** appends to console output using a ref pattern:
```typescript
print(arg) {
  consoleOutRef.current += arg + '\n'
  setConsoleOut(consoleOutRef.current)
}
```

The ref (`consoleOutRef`) is needed because the callback closure captures stale state in React's rendering model. The ref always has the latest value.

**The readLine callback** shows an input prompt and returns a Promise:
```typescript
readLine() {
  return new Promise((resolve) => {
    setInputPrompt(true)        // Show input dialog
    inputResolverRef.current = resolve  // Store resolver for later
  })
}
```

When the user submits the input form, the resolver is called with the input value, which unblocks the WASM interpreter's `readLine()` call.

### Layer 6: CodeMirror Editor (`editor/`)

The editor is built with **CodeMirror 6**, a modular code editor framework. The configuration is in `editor/index.ts`:

```
editor/
  index.ts                  -- Main init, state creation, key bindings
  lang-ulisp/               -- Lisp syntax highlighting
    index.ts                -- Re-exports Clojure Lezer parser
    lang-clojure.ts         -- Lezer grammar definition
    lezer-clojure.js        -- Compiled parser
  rainbow-brackets/         -- Rainbow bracket coloring (currently disabled)
    index.ts
  theme-default/            -- Custom theme
    base.ts                 -- Base theme variables
    light.ts                -- Light variant
    dark.ts                 -- Dark variant (exists but unused)
```

The Lisp syntax highlighting reuses a **Clojure Lezer parser** (`@nextjournal/lang-clojure`). Since uLisp syntax is essentially a subset of Clojure, this works well for highlighting parentheses, symbols, strings, and numbers.

**Parinfer** support is integrated but currently disabled (the UI toggle is commented out). Parinfer is a smart parenthesis management system for Lisp that automatically adjusts indentation and parenthesis placement. When enabled, it would provide three modes: smart, paren, and indent.

**Key bindings:**
- `Ctrl+Enter` / `Cmd+Enter` — Run the code
- `Tab` — Standard indent
- Standard CodeMirror: undo/redo, search, bracket matching, autocomplete


### Layer 7: Build and Dev System

The project uses a modern JavaScript toolchain:

| Tool | Version | Purpose |
|------|---------|---------|
| **Vite** | 6.3.5 | Dev server, bundler, HMR, SSR |
| **Bun** | latest | JS runtime, package manager, script runner |
| **React** | 19.1.0 | UI framework |
| **React Router** | 7.5.3 | Client + server-side routing |
| **Tailwind CSS** | 4.1.5 | Utility-first CSS framework |
| **TypeScript** | 5.8.3 | Type checking |
| **Emscripten** | 4.0.19 (Docker) | C-to-WASM compiler |
| **CodeMirror** | 6.x | Code editor |

**Development mode** (`bun run start`):
```
Vite dev server on http://localhost:5173/
  - Serves React app with HMR
  - Loads WASM from public/ulisp.wasm
  - No Docker needed (uses pre-built WASM)
```

**Production build** (`bun build:site`):
```
1. vite build          -> docs/index.html + JS/CSS bundles
2. vite build --ssr    -> build/server/server.js
3. node build/server/server.js  -> Static HTML generation (SSR)
```

The production build uses Vite's SSR capabilities. React Router renders the page to static HTML on the server side (in Node), which gets written to `docs/` for GitHub Pages deployment. The base path is `/ulisp-wasm` because the site is published at `eliot-akira.github.io/ulisp-wasm`.

**WASM rebuild** (`bun run build:web`):
```
Docker container runs emcc to compile c99/ulisp.c -> web/ulisp.js + public/ulisp.wasm
Requires Docker running
```

---

## 3. File Reference

Every file you might need to touch, organized by subsystem:

### C Interpreter (the WASM core)

| File | Lines | What it does |
|------|-------|-------------|
| `c99/ulisp.c` | 9,006 | Complete uLisp interpreter in C99 |
| `c99/bestline.c` | 4,094 | Readline replacement (only used for native CLI, not WASM) |
| `c99/bestline.h` | — | Header for bestline |

### WASM Build

| File | What it does |
|------|-------------|
| `build.ts` | Build script: emcc commands for web/node/wasi targets, CLI with clang, cross-compile with Zig |
| `docker-compose.yml` | Docker config for Emscripten build environment |
| `web/ulisp.js` | Emscripten-generated JS loader (82 KB) |
| `public/ulisp.wasm` | Compiled WASM binary (1.1 MB) |

### Web Integration

| File | What it does |
|------|-------------|
| `web/web.ts` | Browser wrapper: creates `ulisp` global, provides host callbacks, delegates to `common.js` |
| `web/index.ts` | Re-exports `web.ts` for Worker import |
| `node/common.js` | Core eval logic: `lispCreator()`, `PrintBuffer`, `evaluate()` |

### Site (React Playground)

| File | What it does |
|------|-------------|
| `site/pages/index.tsx` | Main page: editor + console + toolbar |
| `site/lib/create-lisp.ts` | Worker management: `createWorkerRequest()`, `createLisp()` host-side API |
| `site/lib/ulisp-worker.ts` | Worker script: loads WASM, evals code, posts results |
| `site/ui/Layout.tsx` | Layout wrapper with SSR divider |
| `site/routes.tsx` | React Router config (single route `/`) |
| `site/client.tsx` | Client entry: hydrate or render React app |
| `site/server.tsx` | SSR entry: render to static HTML for GitHub Pages |
| `site/constants.ts` | Shared constants (HTML divider for SSR) |
| `site/global.css` | Tailwind CSS import |
| `site/index.scss` | Custom styles |

### Editor

| File | What it does |
|------|-------------|
| `editor/index.ts` | Editor setup: CM6 extensions, key bindings, theme, Parinfer |
| `editor/lang-ulisp/index.ts` | Lisp language support (re-exports Clojure parser) |
| `editor/lang-ulisp/lang-clojure.ts` | Lezer grammar for Clojure/Lisp |
| `editor/lang-ulisp/lezer-clojure.js` | Compiled Lezer parser |
| `editor/theme-default/base.ts` | Base theme (fonts, sizes, gutters) |
| `editor/theme-default/light.ts` | Light color scheme |
| `editor/theme-default/dark.ts` | Dark color scheme (exists, unused) |
| `editor/rainbow-brackets/index.ts` | Rainbow bracket plugin (exists, disabled) |

### Configuration

| File | What it does |
|------|-------------|
| `package.json` | Dependencies, build scripts |
| `tsconfig.json` | TypeScript config, path aliases |
| `vite.config.ts` | Vite config: plugins, base path, SSR settings |
| `index.html` | HTML template |

---

## 4. Building and Running

### Prerequisites

- **Bun** (or npm/node) — for JS/TS tooling
- **Docker** — only if you need to rebuild the WASM binary from C

### Quick Start (no Docker needed)

```bash
cd ulisp-wasm
bun install
bun run start
# Open http://localhost:5173/
```

This starts the Vite dev server with hot module replacement. The pre-built WASM binary (`public/ulisp.wasm`) is used. You can edit any TypeScript or CSS file and changes appear immediately in the browser.

### Rebuilding the WASM Binary

If you change the C source (`c99/ulisp.c`):

```bash
# Requires Docker running
bun run build:web
```

This runs Emscripten inside Docker, producing new `web/ulisp.js` and `public/ulisp.wasm` files. Then restart the dev server.

For a faster C development cycle, there's a watch mode:

```bash
bun run dev:web   # Watches c99/*.c and c99/*.h, rebuilds WASM on change
```

### Running Tests

```bash
bun test          # Run the 640-test uLisp test suite (Node + WASM)
bun run test:dev  # Watch mode
```

The tests are in `tests/index.ts` and verify that the WASM interpreter produces correct results for all uLisp language features.

### Production Build

```bash
bun run build:site
```

This generates the static site in `docs/`, ready for GitHub Pages deployment.


---

## 5. Testing with Playwright

The playground can be tested end-to-end using Playwright. Here is a practical example of loading the playground, entering code, and verifying output:

```typescript
// Example Playwright test
import { test, expect } from '@playwright/test'

test('evaluates basic arithmetic', async ({ page }) => {
  await page.goto('http://localhost:5173/')
  
  // Wait for WASM to load and initial evaluation to complete
  await page.waitForTimeout(3000)
  
  // The editor is a CodeMirror textbox
  const editor = page.locator('[role="textbox"]')
  await editor.click()
  
  // Select all existing code
  await page.keyboard.press('Control+a')
  
  // Type new code
  await page.keyboard.type('(+ 1 2 3 4 5)')
  
  // Run with Ctrl+Enter
  await page.keyboard.press('Control+Enter')
  
  // Wait for output
  await page.waitForTimeout(1000)
  
  // Check console output
  const consoleOutput = page.locator('code')
  await expect(consoleOutput).toContainText('15')
})
```

**Key points for testing:**

- The WASM module takes ~2 seconds to load on first visit. Use `waitForTimeout(3000)` or wait for the step counter to appear.
- CodeMirror 6 uses a `role="textbox"` div. Click it to focus, then use keyboard events to type.
- `Ctrl+a` selects all text in the editor, allowing you to replace the example code.
- `Ctrl+Enter` triggers evaluation.
- Console output appears in a `<code>` element inside a `<pre>` block.
- The Stop button terminates the Worker. After stopping, the next Run automatically creates a new Worker.

---

## 6. Areas for Improvement

### UI Improvements

**1. Canvas/Graphics Output**

The playground has no graphics output. The PicoCalc version supports `draw-pixel`, `draw-line`, `fill-screen`, etc. A `<canvas>` element could be added to the console panel, with WASM-to-host callbacks that translate uLisp graphics calls into Canvas 2D API calls.

Implementation sketch:
```
In c99/ulisp.c:  draw-pixel calls ulisp.call('drawPixel', x, y, color)
In web/web.ts:   ulisp.call() handler calls canvas.getContext('2d').fillRect()
In site/pages:   Add <canvas> element, pass context to createLisp options
```

**2. Better Console Styling**

Errors and normal output currently use the same styling. Errors should be red. The step counter could show a progress bar or animation during evaluation.

**3. Multiple Example Programs**

The playground ships with one hardcoded example (Fibonacci). A dropdown or sidebar with example programs (mandelbrot, sudoku, ray tracing, etc.) from the `examples/` directory would make the playground more engaging.

**4. Dark Theme Toggle**

A dark theme already exists in `editor/theme-default/dark.ts` but is not wired up. A toggle button in the toolbar could switch between light and dark.

### Technical Improvements

**5. Hot-Reload Without Page Refresh**

Currently, changing code in the editor and pressing Run creates a new Worker each time (because `eval()` calls `restart()` if `running` is true). It would be more efficient to reuse the Worker and only re-evaluate.

**6. Persistent Workspace**

The uLisp `(save-image)` function saves the entire Lisp workspace. In the WASM build, this could persist to `localStorage` or IndexedDB, so you can close the browser and resume where you left off.

**7. Better Error Display**

uLisp errors currently appear as plain text in the console. Stack traces, line numbers, and error highlighting in the editor would improve the debugging experience.

**8. Autocomplete**

The native CLI build has autocomplete (via bestline). The web editor could use CodeMirror's `@codemirror/autocomplete` (already installed) to provide autocomplete for uLisp built-in symbols.

### Missing Features (Compared to PicoCalc uLisp)

These features exist in the PicoCalc firmware but not in the WASM playground:

| Feature | PicoCalc | WASM | Gap |
|---------|----------|------|-----|
| Graphics (draw-pixel, etc.) | Yes (TFT_eSPI) | No | Needs canvas host callbacks |
| SD card | Yes (SPI) | No | Could emulate with IndexedDB |
| Keyboard input | Yes (I2C) | Partial | readLine works, but not raw key events |
| Sound (note) | Yes (PWM) | No | Could use Web Audio API |
| ARM assembler | Yes | No | Not compiled in for WASM |
| I2C/SPI hardware | Yes | No | No real hardware in browser |
| save-image / load-image | Yes (flash/SD) | No | Could use localStorage |

---

## 7. API Reference

### Host Callbacks (C → JS)

These are methods on the `ulisp` global object that the C interpreter calls:

| Callback | Signature | When called |
|----------|-----------|------------|
| `writeLine(data)` | `(string) => void` | Interpreter prints output |
| `writeLineError(data)` | `(string) => void` | Interpreter encounters error |
| `readLine()` | `() => Promise<string>` | `(read)` or input prompt |
| `delay(ms)` | `(number) => Promise<void>` | `(delay N)` in Lisp |
| `escape()` | `() => number` | Check if stop requested (return 1 to stop) |
| `wait_for_tick()` | `() => void` | Called on every eval step |
| `call(cmd, ...args)` | `(string, ...any) => any` | Hardware emulation |
| `readByte(streamType)` | `(number) => Promise<number>` | Stream read |
| `writeByte(streamType, data)` | `(number, string) => void` | Stream write |

### WASM Exported Functions (JS → C)

| Function | C signature | What it does |
|----------|------------|-------------|
| `_setup` | `void setup()` | Initialize workspace, must be called once |
| `_evaluate` | `void evaluate(char* code)` | Evaluate a Lisp expression string |
| `_free` | `void free(void* ptr)` | Free WASM memory |
| `_print_version` | `void print_version()` | Print version to output buffer |
| `_stop_loop` | `void stop_loop()` | Signal loop termination |

### Worker Message Protocol

**Main → Worker:**

```typescript
{ id: string, data: { code: string, wasmPath: string } }
```

**Worker → Main (during evaluation):**

```typescript
{ step: number }                    // Step counter update
{ print: string }                   // Normal output line
{ printError: string }              // Error output line
{ action: string, requestId: string, args: any[] }  // Request host action
```

**Worker → Main (on completion):**

```typescript
{ id: string, result: string }      // Eval result (or null)
```

**Main → Worker (action response):**

```typescript
{ action: 'response', id: string, response: any }  // Response to readLine etc.
```

---

## 8. Common Tasks

### Change the default example code

Edit the `exampleCode` constant in `site/pages/index.tsx`:

```typescript
const exampleCode = `(defun fib (n)
  (if (< n 3) 1
    (+ (fib (- n 1)) (fib (- n 2)))))

(fib 5)`
```

### Add a new uLisp built-in function

1. Add the function implementation in `c99/ulisp.c`
2. Add it to the function table and symbol lookup
3. Rebuild WASM: `bun run build:web`
4. Restart the dev server

### Change the editor theme

Edit `editor/theme-default/light.ts` or create a new theme file, then import it in `editor/index.ts`:

```typescript
import { dark } from './theme-default/dark.ts'
// Replace `light` with `dark` in the extensions array
```

### Add a new button to the toolbar

Edit `site/pages/index.tsx`. The toolbar buttons are in the `<div className="ui-toolbar">` section. Follow the pattern of the existing Run/Stop/Share buttons.

