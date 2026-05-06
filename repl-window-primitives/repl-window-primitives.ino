// PicoCalc REPL Window Primitive Experiment
//
// Purpose:
//   Exercise the PicoCalc TFT display and keyboard with a small C++-only
//   Arduino sketch before integrating the REPL window model into uLisp.
//
// What this sketch intentionally does NOT include:
//   - uLisp object system
//   - reader/evaluator/printer
//   - garbage collector
//   - stream dispatch
//
// What this sketch DOES include:
//   - TFT screen initialization
//   - PCKeyboard I2C initialization on Wire1 SDA=6/SCL=7
//   - append-only transcript/back buffer
//   - editable input line with cursor
//   - key-event logging for discovering special key codes
//   - dirty character-cell renderer: transcript + status + input line

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <TFT_eSPI.h>
#include <PCKeyboard.h>

TFT_eSPI tft = TFT_eSPI();
PCKeyboard pc_kbd;

// -----------------------------------------------------------------------------
// Display geometry
// -----------------------------------------------------------------------------

constexpr int ScreenWidth = 320;
constexpr int ScreenHeight = 320;
constexpr int CharWidth = 6;
constexpr int CharHeight = 8;
constexpr int Leading = 10;
constexpr int Columns = ScreenWidth / CharWidth;  // 53
constexpr int Lines = ScreenHeight / Leading;     // 32
constexpr int StatusRows = 1;
constexpr int InputRows = 3;
constexpr int TranscriptRows = Lines - StatusRows - InputRows;
constexpr int LastTranscriptRow = TranscriptRows - 1;

// Tune this first on hardware. 160 * 53 bytes = 8,480 bytes.
constexpr int BackBufferRows = 160;
constexpr int InputBufferSize = 512;

// Existing uLisp/PicoCalc code treats these as special/non-printable keyboard
// codes. This sketch logs any non-printable key so we can confirm mappings.
constexpr uint8_t KeyEsc = 0xB1;
constexpr uint8_t KeyCandidateLeft = 0xA1;
constexpr uint8_t KeyCandidateRight = 0xA2;
constexpr uint8_t KeyCandidateUp = 0xA3;
constexpr uint8_t KeyCandidateDown = 0xA4;
constexpr uint8_t KeyCandidateDel = 0xA5;

// -----------------------------------------------------------------------------
// Back buffer: physical display rows in a circular transcript
// -----------------------------------------------------------------------------

struct BackBufferState {
  char rows[BackBufferRows][Columns];
  uint16_t currentRow = 0;     // physical row currently being appended to
  uint16_t currentCol = 0;
  uint16_t count = 1;          // valid rows, including the current row
  uint16_t viewportStart = 0;  // logical row shown at top of transcript area
  bool followTail = true;
};

BackBufferState backBuffer;

uint16_t logicalFirstRow() {
  if (backBuffer.count < BackBufferRows) return 0;
  return (backBuffer.currentRow + 1) % BackBufferRows;
}

uint16_t physicalRowForLogical(uint16_t logicalIndex) {
  const uint16_t first = logicalFirstRow();
  return (first + logicalIndex) % BackBufferRows;
}

void clearBackRow(uint16_t physicalRow) {
  for (int col = 0; col < Columns; col++) {
    backBuffer.rows[physicalRow][col] = ' ';
  }
}

void scrollBackBufferToTail() {
  if (backBuffer.count <= TranscriptRows) {
    backBuffer.viewportStart = 0;
  } else {
    backBuffer.viewportStart = backBuffer.count - TranscriptRows;
  }
}

void advanceBackRow() {
  backBuffer.currentRow = (backBuffer.currentRow + 1) % BackBufferRows;
  clearBackRow(backBuffer.currentRow);
  backBuffer.currentCol = 0;
  if (backBuffer.count < BackBufferRows) backBuffer.count++;
  if (backBuffer.followTail) scrollBackBufferToTail();
}

void appendBackChar(char c) {
  if (c == '\r') return;
  if (c == '\n') {
    advanceBackRow();
    return;
  }
  if ((static_cast<uint8_t>(c) & 0x7F) < 32) return;

  backBuffer.rows[backBuffer.currentRow][backBuffer.currentCol++] = c;
  if (backBuffer.currentCol >= Columns) {
    advanceBackRow();
  }
  if (backBuffer.followTail) scrollBackBufferToTail();
}

void appendBackString(const char *s) {
  while (*s) appendBackChar(*s++);
}

void appendBackUnsigned(uint32_t value) {
  char buf[12];
  snprintf(buf, sizeof(buf), "%lu", static_cast<unsigned long>(value));
  appendBackString(buf);
}

void appendBackHex8(uint8_t value) {
  char buf[5];
  snprintf(buf, sizeof(buf), "0x%02X", value);
  appendBackString(buf);
}

void appendBackLine(const char *s) {
  appendBackString(s);
  appendBackChar('\n');
}

void clearBackBuffer() {
  for (int row = 0; row < BackBufferRows; row++) clearBackRow(row);
  backBuffer.currentRow = 0;
  backBuffer.currentCol = 0;
  backBuffer.count = 1;
  backBuffer.viewportStart = 0;
  backBuffer.followTail = true;
}

// -----------------------------------------------------------------------------
// Edit buffer: mutable active input line
// -----------------------------------------------------------------------------

struct EditBufferState {
  char text[InputBufferSize];
  uint16_t len = 0;
  uint16_t cursor = 0;
};

EditBufferState editBuffer;
bool uiDirty = true;

void requestRender() {
  uiDirty = true;
}

void resetEditBuffer() {
  editBuffer.len = 0;
  editBuffer.cursor = 0;
  editBuffer.text[0] = '\0';
}

bool insertEditChar(char c) {
  if (editBuffer.len >= InputBufferSize - 1) return false;
  for (int i = editBuffer.len; i > editBuffer.cursor; i--) {
    editBuffer.text[i] = editBuffer.text[i - 1];
  }
  editBuffer.text[editBuffer.cursor] = c;
  editBuffer.cursor++;
  editBuffer.len++;
  editBuffer.text[editBuffer.len] = '\0';
  return true;
}

bool backspaceEditChar() {
  if (editBuffer.cursor == 0) return false;
  for (int i = editBuffer.cursor - 1; i < editBuffer.len - 1; i++) {
    editBuffer.text[i] = editBuffer.text[i + 1];
  }
  editBuffer.cursor--;
  editBuffer.len--;
  editBuffer.text[editBuffer.len] = '\0';
  return true;
}

bool deleteEditChar() {
  if (editBuffer.cursor >= editBuffer.len) return false;
  for (int i = editBuffer.cursor; i < editBuffer.len - 1; i++) {
    editBuffer.text[i] = editBuffer.text[i + 1];
  }
  editBuffer.len--;
  editBuffer.text[editBuffer.len] = '\0';
  return true;
}

void moveCursorLeft() {
  if (editBuffer.cursor > 0) editBuffer.cursor--;
}

void moveCursorRight() {
  if (editBuffer.cursor < editBuffer.len) editBuffer.cursor++;
}

void runFakeEvaluator(const char *input) {
  if (strcmp(input, "/help") == 0) {
    appendBackLine("commands: /help /spam /clear /status");
    appendBackLine("normal text is echoed as fake evaluator output");
    return;
  }

  if (strcmp(input, "/spam") == 0) {
    for (int i = 0; i < 48; i++) {
      appendBackString("spam line ");
      appendBackUnsigned(i);
      appendBackString(": abcdefghijklmnopqrstuvwxyz 0123456789");
      appendBackChar('\n');
    }
    return;
  }

  if (strcmp(input, "/clear") == 0) {
    clearBackBuffer();
    appendBackLine("back buffer cleared");
    return;
  }

  if (strcmp(input, "/status") == 0) {
    appendBackString("status rows=");
    appendBackUnsigned(backBuffer.count);
    appendBackString(" viewport=");
    appendBackUnsigned(backBuffer.viewportStart);
    appendBackString(" input-len=");
    appendBackUnsigned(editBuffer.len);
    appendBackChar('\n');
    return;
  }

  appendBackString("fake-eval: ");
  appendBackString(input);
  appendBackChar('\n');
}

void commitEditBuffer() {
  char committed[InputBufferSize];
  strncpy(committed, editBuffer.text, sizeof(committed));
  committed[sizeof(committed) - 1] = '\0';

  appendBackString("> ");
  appendBackString(committed);
  appendBackChar('\n');
  runFakeEvaluator(committed);
  resetEditBuffer();
}

// -----------------------------------------------------------------------------
// Renderer
// -----------------------------------------------------------------------------
//
// Flicker note:
//   Do not clear and redraw the full TFT on every render. The TFT is updated
//   directly over SPI, so a row clear is visible before the replacement text is
//   drawn. Instead we build a desired character-cell frame in RAM, compare it to
//   the last frame sent to the display, and draw only changed cells.
//
// RAM note:
//   A full 320x320 RGB565 framebuffer would cost 204,800 bytes. That is too
//   expensive for uLisp. This renderer stores only text cells: char + 1-byte
//   attribute. Two 32x53 cell buffers cost 6,784 bytes total.

enum CellAttr : uint8_t {
  AttrNormal = 0,
  AttrStatus = 1,
  AttrPrompt = 2,
  AttrCursor = 3,
};

struct RenderCell {
  char ch;
  uint8_t attr;
};

RenderCell desiredCells[Lines][Columns];
RenderCell drawnCells[Lines][Columns];
bool drawnCellsValid = false;

uint16_t attrFg(uint8_t attr) {
  if (attr == AttrStatus) return TFT_BLACK;
  if (attr == AttrPrompt) return TFT_GREEN;
  if (attr == AttrCursor) return TFT_BLACK;
  return TFT_WHITE;
}

uint16_t attrBg(uint8_t attr) {
  if (attr == AttrStatus) return TFT_DARKGREY;
  if (attr == AttrCursor) return TFT_GREEN;
  return TFT_BLACK;
}

bool cellsEqualAt(int row, int col) {
  return desiredCells[row][col].ch == drawnCells[row][col].ch &&
         desiredCells[row][col].attr == drawnCells[row][col].attr;
}

void setDesiredCell(int row, int col, char ch, uint8_t attr = AttrNormal) {
  if (row < 0 || row >= Lines || col < 0 || col >= Columns) return;
  desiredCells[row][col].ch = ch;
  desiredCells[row][col].attr = attr;
}

void clearDesiredCells() {
  for (int row = 0; row < Lines; row++) {
    for (int col = 0; col < Columns; col++) {
      setDesiredCell(row, col, ' ', AttrNormal);
    }
  }
}

void setDesiredTextClipped(int row, int col, const char *text, uint8_t attr = AttrNormal) {
  while (*text && col < Columns) {
    setDesiredCell(row, col++, *text++, attr);
  }
}

void drawCellDirect(int row, int col) {
  const RenderCell &cell = desiredCells[row][col];
  const int x = col * CharWidth;
  const int y = row * Leading;
  const uint16_t bg = attrBg(cell.attr);
  tft.fillRect(x, y, CharWidth, Leading, bg);
  if (cell.ch != ' ') {
    tft.drawChar(x, y, cell.ch, attrFg(cell.attr), bg, 1);
  }
}

void composeTranscript() {
  for (int row = 0; row < TranscriptRows; row++) {
    const uint16_t logical = backBuffer.viewportStart + row;
    if (logical >= backBuffer.count) continue;
    const uint16_t physical = physicalRowForLogical(logical);
    for (int col = 0; col < Columns; col++) {
      const char ch = backBuffer.rows[physical][col];
      if (ch != ' ') setDesiredCell(row, col, ch, AttrNormal);
    }
  }
}

void composeStatus() {
  const int row = TranscriptRows;
  for (int col = 0; col < Columns; col++) {
    setDesiredCell(row, col, ' ', AttrStatus);
  }
  char status[80];
  snprintf(status, sizeof(status), "rows %u/%u view %u cursor %u len %u",
           backBuffer.count,
           BackBufferRows,
           backBuffer.viewportStart,
           editBuffer.cursor,
           editBuffer.len);
  setDesiredTextClipped(row, 0, status, AttrStatus);
}

void composeInput() {
  const int firstRow = TranscriptRows + StatusRows;
  setDesiredTextClipped(firstRow, 0, "> ", AttrPrompt);

  int screenRow = firstRow;
  int screenCol = 2;
  for (uint16_t i = 0; i < editBuffer.len && screenRow < Lines; i++) {
    if (screenCol >= Columns) {
      screenCol = 0;
      screenRow++;
      if (screenRow >= Lines) break;
    }
    setDesiredCell(screenRow, screenCol, editBuffer.text[i], AttrNormal);
    screenCol++;
  }

  const uint16_t cursorVisual = editBuffer.cursor + 2;
  const int cursorRow = firstRow + (cursorVisual / Columns);
  const int cursorCol = cursorVisual % Columns;
  if (cursorRow < Lines) {
    char underCursor = ' ';
    if (editBuffer.cursor < editBuffer.len) underCursor = editBuffer.text[editBuffer.cursor];
    setDesiredCell(cursorRow, cursorCol, underCursor == ' ' ? '_' : underCursor, AttrCursor);
  }
}

void renderAll() {
  clearDesiredCells();
  composeTranscript();
  composeStatus();
  composeInput();

  for (int row = 0; row < Lines; row++) {
    for (int col = 0; col < Columns; col++) {
      if (!drawnCellsValid || !cellsEqualAt(row, col)) {
        drawCellDirect(row, col);
        drawnCells[row][col] = desiredCells[row][col];
      }
    }
  }
  drawnCellsValid = true;
}

// -----------------------------------------------------------------------------
// Keyboard handling
// -----------------------------------------------------------------------------

void logKeyEvent(uint8_t key, PCKeyboard::KeyState state) {
  appendBackString("key ");
  appendBackHex8(key);
  appendBackString(" state ");
  appendBackUnsigned(static_cast<uint8_t>(state));
  appendBackChar('\n');
}

void processPrintableOrControl(uint8_t key) {
  if (key == KeyEsc) {
    appendBackLine("ESC pressed");
    resetEditBuffer();
    requestRender();
    return;
  }

  if (key == '\r' || key == '\n') {
    commitEditBuffer();
    requestRender();
    return;
  }

  if (key == 8 || key == 0x7F) {
    backspaceEditChar();
    requestRender();
    return;
  }

  // Candidate mappings from existing uLisp filtering. Confirm on hardware.
  if (key == KeyCandidateLeft) {
    moveCursorLeft();
    appendBackLine("candidate LEFT");
    requestRender();
    return;
  }
  if (key == KeyCandidateRight) {
    moveCursorRight();
    appendBackLine("candidate RIGHT");
    requestRender();
    return;
  }
  if (key == KeyCandidateDel) {
    deleteEditChar();
    appendBackLine("candidate DELETE");
    requestRender();
    return;
  }
  if (key == KeyCandidateUp) {
    if (backBuffer.viewportStart > 0) {
      backBuffer.viewportStart--;
      backBuffer.followTail = false;
    }
    appendBackLine("candidate UP / scroll up");
    requestRender();
    return;
  }
  if (key == KeyCandidateDown) {
    if (backBuffer.viewportStart + TranscriptRows < backBuffer.count) {
      backBuffer.viewportStart++;
    } else {
      backBuffer.followTail = true;
      scrollBackBufferToTail();
    }
    appendBackLine("candidate DOWN / scroll down");
    requestRender();
    return;
  }

  if (key >= 32 && key < 127) {
    insertEditChar(static_cast<char>(key));
    requestRender();
    return;
  }

  logKeyEvent(key, PCKeyboard::StatePress);
  requestRender();
}

void pollKeyboard() {
  while (pc_kbd.keyCount() > 0) {
    const PCKeyboard::KeyEvent event = pc_kbd.keyEvent();
    const uint8_t key = static_cast<uint8_t>(event.key);
    if (event.state == PCKeyboard::StatePress) {
      processPrintableOrControl(key);
    } else if (key != 0) {
      logKeyEvent(key, event.state);
      requestRender();
    }
  }
}

// -----------------------------------------------------------------------------
// Arduino entry points
// -----------------------------------------------------------------------------

void initKeyboard() {
  Wire1.setSDA(6);
  Wire1.setSCL(7);
  Wire1.begin();
  Wire1.setClock(10000);
  pc_kbd.begin(0x1f, &Wire1);
}

void initDisplay() {
  tft.init();
  tft.writecommand(TFT_DISPOFF);
  tft.invertDisplay(1);
  tft.fillScreen(TFT_BLACK);
  tft.writecommand(TFT_DISPON);
}

void setup() {
  Serial.begin(115200);
  delay(500);

  clearBackBuffer();
  resetEditBuffer();

  initDisplay();
  initKeyboard();

  appendBackLine("PicoCalc REPL primitive sketch");
  appendBackLine("Type text. Enter commits. Arrows are logged/tested.");
  appendBackLine("Commands: /help /spam /clear /status");
  renderAll();
  uiDirty = false;
}

void loop() {
  pollKeyboard();
  if (uiDirty) {
    renderAll();
    uiDirty = false;
  }
  delay(20);
}
