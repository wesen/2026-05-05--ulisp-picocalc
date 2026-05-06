// PicoCalc REPL Window Primitive Experiment
//
// C++-only Arduino sketch for exercising PicoCalc display + keyboard
// primitives before integrating a REPL window into uLisp.
//
// Features:
//   - TFT_eSPI display init
//   - PCKeyboard init on Wire1 SDA=6/SCL=7 addr 0x1f
//   - scrollback/back-buffer transcript
//   - editable input line with cursor
//   - dirty character-cell renderer (no full-screen redraw flicker)
//   - 16-color neon/neotokyo palette
//   - foreground/background/bold text state
//   - simple icon drawing commands
//   - line/rect/circle/fill graphics commands
//   - /demo command that exercises everything

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <TFT_eSPI.h>
#include <PCKeyboard.h>

TFT_eSPI tft = TFT_eSPI();
PCKeyboard pc_kbd;

// -----------------------------------------------------------------------------
// Geometry and constants
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
constexpr int BackBufferRows = 160;
constexpr int InputBufferSize = 512;
constexpr int GfxCmdMax = 16;

constexpr uint8_t KeyEsc = 0xB1;
constexpr uint8_t KeyCandidateLeft = 0xA1;
constexpr uint8_t KeyCandidateRight = 0xA2;
constexpr uint8_t KeyCandidateUp = 0xA3;
constexpr uint8_t KeyCandidateDown = 0xA4;
constexpr uint8_t KeyCandidateDel = 0xA5;

// -----------------------------------------------------------------------------
// Palette and attributes
// -----------------------------------------------------------------------------

const uint16_t Palette[16] = {
  0x0000,  //  0 void black
  0xCE79,  //  1 soft white
  0xF81F,  //  2 neon pink
  0x07E0,  //  3 neon green
  0x07FF,  //  4 neon cyan
  0xFFE0,  //  5 neon yellow
  0xD81F,  //  6 neon purple
  0xFD20,  //  7 neon orange
  0x001F,  //  8 electric blue
  0xF8B9,  //  9 hot magenta
  0xA7E4,  // 10 acid lime
  0x87FF,  // 11 ice blue
  0xFC80,  // 12 coral
  0x05BF,  // 13 deep cyan
  0xBA9A,  // 14 muted pink
  0x10A2,  // 15 dark navy
};

uint8_t packAttr(uint8_t fg, uint8_t bg) {
  return ((fg & 0x0F) << 4) | (bg & 0x0F);
}

uint8_t attrFgIndex(uint8_t attr) { return (attr >> 4) & 0x0F; }
uint8_t attrBgIndex(uint8_t attr) { return attr & 0x0F; }

int colorNameToIndex(const char *name) {
  if (strcmp(name, "black") == 0) return 0;
  if (strcmp(name, "white") == 0) return 1;
  if (strcmp(name, "pink") == 0) return 2;
  if (strcmp(name, "green") == 0) return 3;
  if (strcmp(name, "cyan") == 0) return 4;
  if (strcmp(name, "yellow") == 0) return 5;
  if (strcmp(name, "purple") == 0) return 6;
  if (strcmp(name, "orange") == 0) return 7;
  if (strcmp(name, "blue") == 0) return 8;
  if (strcmp(name, "magenta") == 0) return 9;
  if (strcmp(name, "lime") == 0) return 10;
  if (strcmp(name, "ice") == 0) return 11;
  if (strcmp(name, "coral") == 0) return 12;
  if (strcmp(name, "deep") == 0) return 13;
  if (strcmp(name, "muted") == 0) return 14;
  if (strcmp(name, "navy") == 0) return 15;
  return -1;
}

int parseColorArg(const char *arg) {
  if (!arg || !*arg) return -1;
  if (arg[0] >= '0' && arg[0] <= '9') {
    int v = atoi(arg);
    if (v >= 0 && v < 16) return v;
    return -1;
  }
  return colorNameToIndex(arg);
}

// -----------------------------------------------------------------------------
// State structs and globals
// -----------------------------------------------------------------------------

struct DrawState {
  uint8_t fg;
  uint8_t bg;
  bool bold;
};

struct BackBufferState {
  char rows[BackBufferRows][Columns];
  uint8_t attrs[BackBufferRows][Columns];
  bool bold[BackBufferRows][Columns];
  uint16_t currentRow;
  uint16_t currentCol;
  uint16_t count;
  uint16_t viewportStart;
  bool followTail;
};

struct EditBufferState {
  char text[InputBufferSize];
  uint16_t len;
  uint16_t cursor;
};

struct RenderCell {
  char ch;
  uint8_t attr;
  bool bold;
};

struct GfxCmd {
  uint8_t type;
  int x0;
  int y0;
  int x1;
  int y1;
  int r;
  uint8_t col;
};

BackBufferState backBuffer;
EditBufferState editBuffer;
DrawState drawState;
RenderCell desiredCells[Lines][Columns];
RenderCell drawnCells[Lines][Columns];
GfxCmd gfxCmds[GfxCmdMax];

bool uiDirty = true;
bool drawnCellsValid = false;
int gfxCmdCount = 0;

// Graphics command types.
constexpr uint8_t GfxLine = 0;
constexpr uint8_t GfxRect = 1;
constexpr uint8_t GfxFillRect = 2;
constexpr uint8_t GfxCircle = 3;
constexpr uint8_t GfxFillCircle = 4;
constexpr uint8_t GfxIcon = 5;

// -----------------------------------------------------------------------------
// Utility
// -----------------------------------------------------------------------------

void requestRender() { uiDirty = true; }

void resetDrawState() {
  drawState.fg = 1;
  drawState.bg = 0;
  drawState.bold = false;
}

void appendBackUnsigned(uint32_t value);
void appendBackString(const char *s);
void appendBackChar(char c);
void appendBackLine(const char *s);

// -----------------------------------------------------------------------------
// Back buffer
// -----------------------------------------------------------------------------

uint16_t logicalFirstRow() {
  if (backBuffer.count < BackBufferRows) return 0;
  return (backBuffer.currentRow + 1) % BackBufferRows;
}

uint16_t physicalRowForLogical(uint16_t logicalIndex) {
  return (logicalFirstRow() + logicalIndex) % BackBufferRows;
}

void clearBackRow(uint16_t row) {
  for (int col = 0; col < Columns; col++) {
    backBuffer.rows[row][col] = ' ';
    backBuffer.attrs[row][col] = packAttr(1, 0);
    backBuffer.bold[row][col] = false;
  }
}

void scrollBackBufferToTail() {
  if (backBuffer.count <= TranscriptRows) backBuffer.viewportStart = 0;
  else backBuffer.viewportStart = backBuffer.count - TranscriptRows;
}

void advanceBackRow() {
  backBuffer.currentRow = (backBuffer.currentRow + 1) % BackBufferRows;
  clearBackRow(backBuffer.currentRow);
  backBuffer.currentCol = 0;
  if (backBuffer.count < BackBufferRows) backBuffer.count++;
  if (backBuffer.followTail) scrollBackBufferToTail();
}

void clearBackBuffer() {
  for (int row = 0; row < BackBufferRows; row++) clearBackRow(row);
  backBuffer.currentRow = 0;
  backBuffer.currentCol = 0;
  backBuffer.count = 1;
  backBuffer.viewportStart = 0;
  backBuffer.followTail = true;
  resetDrawState();
  requestRender();
}

void appendBackChar(char c) {
  if (c == '\r') return;
  if (c == '\n') {
    advanceBackRow();
    requestRender();
    return;
  }
  if ((static_cast<uint8_t>(c) & 0x7F) < 32) return;

  backBuffer.rows[backBuffer.currentRow][backBuffer.currentCol] = c;
  backBuffer.attrs[backBuffer.currentRow][backBuffer.currentCol] = packAttr(drawState.fg, drawState.bg);
  backBuffer.bold[backBuffer.currentRow][backBuffer.currentCol] = drawState.bold;
  backBuffer.currentCol++;
  if (backBuffer.currentCol >= Columns) advanceBackRow();
  if (backBuffer.followTail) scrollBackBufferToTail();
  requestRender();
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

// -----------------------------------------------------------------------------
// Edit buffer
// -----------------------------------------------------------------------------

void resetEditBuffer() {
  editBuffer.len = 0;
  editBuffer.cursor = 0;
  editBuffer.text[0] = '\0';
}

bool insertEditChar(char c) {
  if (editBuffer.len >= InputBufferSize - 1) return false;
  for (int i = editBuffer.len; i > editBuffer.cursor; i--) editBuffer.text[i] = editBuffer.text[i - 1];
  editBuffer.text[editBuffer.cursor++] = c;
  editBuffer.text[++editBuffer.len] = '\0';
  return true;
}

bool backspaceEditChar() {
  if (editBuffer.cursor == 0) return false;
  for (int i = editBuffer.cursor - 1; i < editBuffer.len - 1; i++) editBuffer.text[i] = editBuffer.text[i + 1];
  editBuffer.cursor--;
  editBuffer.text[--editBuffer.len] = '\0';
  return true;
}

bool deleteEditChar() {
  if (editBuffer.cursor >= editBuffer.len) return false;
  for (int i = editBuffer.cursor; i < editBuffer.len - 1; i++) editBuffer.text[i] = editBuffer.text[i + 1];
  editBuffer.text[--editBuffer.len] = '\0';
  return true;
}

void moveCursorLeft() { if (editBuffer.cursor > 0) editBuffer.cursor--; }
void moveCursorRight() { if (editBuffer.cursor < editBuffer.len) editBuffer.cursor++; }

// -----------------------------------------------------------------------------
// Graphics and icons
// -----------------------------------------------------------------------------

void clearGfxCmds() {
  gfxCmdCount = 0;
  tft.fillScreen(TFT_BLACK);
  drawnCellsValid = false;
  requestRender();
}

void addGfxCmd(uint8_t type, int x0, int y0, int x1, int y1, int r, int col) {
  if (gfxCmdCount >= GfxCmdMax) return;
  gfxCmds[gfxCmdCount].type = type;
  gfxCmds[gfxCmdCount].x0 = x0;
  gfxCmds[gfxCmdCount].y0 = y0;
  gfxCmds[gfxCmdCount].x1 = x1;
  gfxCmds[gfxCmdCount].y1 = y1;
  gfxCmds[gfxCmdCount].r = r;
  gfxCmds[gfxCmdCount].col = col & 0x0F;
  gfxCmdCount++;
}

void drawIconPixels(const char *name, int x, int y, uint16_t color) {
  // Tiny hand-drawn icon approximations. Enough to test pixel/icon drawing.
  if (strcmp(name, "heart") == 0) {
    tft.fillCircle(x + 4, y + 4, 4, color);
    tft.fillCircle(x + 12, y + 4, 4, color);
    tft.fillTriangle(x, y + 5, x + 16, y + 5, x + 8, y + 18, color);
  } else if (strcmp(name, "star") == 0) {
    tft.drawLine(x + 8, y, x + 8, y + 16, color);
    tft.drawLine(x, y + 8, x + 16, y + 8, color);
    tft.drawLine(x + 2, y + 2, x + 14, y + 14, color);
    tft.drawLine(x + 14, y + 2, x + 2, y + 14, color);
  } else if (strcmp(name, "check") == 0) {
    tft.drawLine(x, y + 8, x + 5, y + 14, color);
    tft.drawLine(x + 5, y + 14, x + 16, y, color);
  } else if (strcmp(name, "cross") == 0) {
    tft.drawLine(x, y, x + 16, y + 16, color);
    tft.drawLine(x + 16, y, x, y + 16, color);
  } else if (strcmp(name, "left") == 0) {
    tft.fillTriangle(x, y + 8, x + 12, y, x + 12, y + 16, color);
  } else if (strcmp(name, "right") == 0) {
    tft.fillTriangle(x + 16, y + 8, x + 4, y, x + 4, y + 16, color);
  } else {
    tft.fillRect(x, y, 16, 16, color);
  }
}

void applyGfxCmdIndex(int i) {
  uint16_t col = Palette[gfxCmds[i].col & 0x0F];
  switch (gfxCmds[i].type) {
    case GfxLine:       tft.drawLine(gfxCmds[i].x0, gfxCmds[i].y0, gfxCmds[i].x1, gfxCmds[i].y1, col); break;
    case GfxRect:       tft.drawRect(gfxCmds[i].x0, gfxCmds[i].y0, gfxCmds[i].x1, gfxCmds[i].y1, col); break;
    case GfxFillRect:   tft.fillRect(gfxCmds[i].x0, gfxCmds[i].y0, gfxCmds[i].x1, gfxCmds[i].y1, col); break;
    case GfxCircle:     tft.drawCircle(gfxCmds[i].x0, gfxCmds[i].y0, gfxCmds[i].r, col); break;
    case GfxFillCircle: tft.fillCircle(gfxCmds[i].x0, gfxCmds[i].y0, gfxCmds[i].r, col); break;
    case GfxIcon:       drawIconPixels("heart", gfxCmds[i].x0, gfxCmds[i].y0, col); break;
  }
}

void replayGfxCmds() {
  for (int i = 0; i < gfxCmdCount; i++) applyGfxCmdIndex(i);
}

void drawGfxLine(int x0, int y0, int x1, int y1, int col) {
  addGfxCmd(GfxLine, x0, y0, x1, y1, 0, col);
  applyGfxCmdIndex(gfxCmdCount - 1);
}

void drawGfxRect(int x, int y, int w, int h, int col) {
  addGfxCmd(GfxRect, x, y, w, h, 0, col);
  applyGfxCmdIndex(gfxCmdCount - 1);
}

void drawGfxFillRect(int x, int y, int w, int h, int col) {
  addGfxCmd(GfxFillRect, x, y, w, h, 0, col);
  applyGfxCmdIndex(gfxCmdCount - 1);
}

void drawGfxCircle(int x, int y, int r, int col) {
  addGfxCmd(GfxCircle, x, y, 0, 0, r, col);
  applyGfxCmdIndex(gfxCmdCount - 1);
}

void drawGfxFillCircle(int x, int y, int r, int col) {
  addGfxCmd(GfxFillCircle, x, y, 0, 0, r, col);
  applyGfxCmdIndex(gfxCmdCount - 1);
}

void drawIconCommand(const char *name, int x, int y, int col) {
  uint16_t color = Palette[col & 0x0F];
  drawIconPixels(name, x, y, color);
  // Store as generic icon replay. Replay always heart for now; command text keeps the name.
  addGfxCmd(GfxIcon, x, y, 0, 0, 0, col);
}

// -----------------------------------------------------------------------------
// Renderer
// -----------------------------------------------------------------------------

uint16_t cellFgColor(int row, int col) {
  return Palette[attrFgIndex(desiredCells[row][col].attr)];
}

uint16_t cellBgColor(int row, int col) {
  return Palette[attrBgIndex(desiredCells[row][col].attr)];
}

bool cellsEqualAt(int row, int col) {
  return desiredCells[row][col].ch == drawnCells[row][col].ch &&
         desiredCells[row][col].attr == drawnCells[row][col].attr &&
         desiredCells[row][col].bold == drawnCells[row][col].bold;
}

void setDesiredCell(int row, int col, char ch, uint8_t attr, bool bold) {
  if (row < 0 || row >= Lines || col < 0 || col >= Columns) return;
  desiredCells[row][col].ch = ch;
  desiredCells[row][col].attr = attr;
  desiredCells[row][col].bold = bold;
}

void clearDesiredCells() {
  for (int row = 0; row < Lines; row++) {
    for (int col = 0; col < Columns; col++) setDesiredCell(row, col, ' ', packAttr(1, 0), false);
  }
}

void setDesiredText(int row, int col, const char *text, uint8_t attr, bool bold) {
  while (*text && col < Columns) setDesiredCell(row, col++, *text++, attr, bold);
}

void drawCellDirect(int row, int col) {
  int x = col * CharWidth;
  int y = row * Leading;
  uint16_t fg = cellFgColor(row, col);
  uint16_t bg = cellBgColor(row, col);
  char ch = desiredCells[row][col].ch;
  tft.fillRect(x, y, CharWidth, Leading, bg);
  if (ch != ' ') {
    tft.drawChar(x, y, ch, fg, bg, 1);
    if (desiredCells[row][col].bold && x + 1 < ScreenWidth) tft.drawChar(x + 1, y, ch, fg, bg, 1);
  }
}

void composeTranscript() {
  for (int row = 0; row < TranscriptRows; row++) {
    uint16_t logical = backBuffer.viewportStart + row;
    if (logical >= backBuffer.count) continue;
    uint16_t physical = physicalRowForLogical(logical);
    for (int col = 0; col < Columns; col++) {
      char ch = backBuffer.rows[physical][col];
      if (ch != ' ') setDesiredCell(row, col, ch, backBuffer.attrs[physical][col], backBuffer.bold[physical][col]);
    }
  }
}

void composeStatus() {
  int row = TranscriptRows;
  uint8_t attr = packAttr(0, 11); // black on ice blue
  for (int col = 0; col < Columns; col++) setDesiredCell(row, col, ' ', attr, false);
  char status[96];
  snprintf(status, sizeof(status), "rows %u/%u view %u cursor %u len %u fg %u bg %u bold %u gfx %d/%d",
           backBuffer.count, BackBufferRows, backBuffer.viewportStart,
           editBuffer.cursor, editBuffer.len, drawState.fg, drawState.bg,
           drawState.bold, gfxCmdCount, GfxCmdMax);
  setDesiredText(row, 0, status, attr, false);
}

void composeInput() {
  int firstRow = TranscriptRows + StatusRows;
  setDesiredText(firstRow, 0, "> ", packAttr(3, 0), false);
  int screenRow = firstRow;
  int screenCol = 2;
  for (uint16_t i = 0; i < editBuffer.len && screenRow < Lines; i++) {
    if (screenCol >= Columns) {
      screenCol = 0;
      screenRow++;
      if (screenRow >= Lines) break;
    }
    setDesiredCell(screenRow, screenCol++, editBuffer.text[i], packAttr(1, 0), false);
  }
  uint16_t cursorVisual = editBuffer.cursor + 2;
  int cursorRow = firstRow + (cursorVisual / Columns);
  int cursorCol = cursorVisual % Columns;
  if (cursorRow < Lines) {
    char under = ' ';
    if (editBuffer.cursor < editBuffer.len) under = editBuffer.text[editBuffer.cursor];
    setDesiredCell(cursorRow, cursorCol, under == ' ' ? '_' : under, packAttr(0, 3), false);
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
  replayGfxCmds();
}

// -----------------------------------------------------------------------------
// Fake evaluator and commands
// -----------------------------------------------------------------------------

void printPalette() {
  appendBackLine("palette:");
  for (int i = 0; i < 16; i++) {
    uint8_t oldFg = drawState.fg;
    uint8_t oldBg = drawState.bg;
    drawState.fg = i;
    drawState.bg = 0;
    appendBackUnsigned(i);
    appendBackString(" fg block ");
    drawState.fg = 1;
    drawState.bg = i;
    appendBackString(" bg block ");
    appendBackChar('\n');
    drawState.fg = oldFg;
    drawState.bg = oldBg;
  }
}

void runDemo() {
  appendBackLine("demo: palette + bold + graphics");
  printPalette();
  appendBackString("normal ");
  drawState.bold = true;
  appendBackString("BOLD ");
  drawState.bold = false;
  drawState.fg = 2; appendBackString("pink ");
  drawState.fg = 4; appendBackString("cyan ");
  drawState.fg = 3; appendBackString("green");
  resetDrawState();
  appendBackChar('\n');

  clearGfxCmds();
  drawGfxFillRect(0, 0, 320, 320, 15);
  drawGfxFillCircle(160, 160, 60, 4);
  drawGfxCircle(160, 160, 80, 2);
  drawGfxCircle(160, 160, 100, 6);
  drawGfxLine(0, 0, 319, 319, 3);
  drawGfxLine(319, 0, 0, 319, 7);
  drawGfxFillRect(20, 20, 50, 30, 8);
  drawGfxFillRect(250, 270, 50, 30, 12);
  drawGfxRect(80, 80, 160, 160, 5);
  drawIconCommand("heart", 145, 145, 2);
  appendBackLine("graphics demo drawn; /clear-gfx removes overlay");
}

void runFakeEvaluator(const char *input) {
  if (strcmp(input, "/help") == 0) {
    appendBackLine("commands: /help /demo /spam /clear /status");
    appendBackLine("/fg N|name /bg N|name /bold [0|1] /normal /palette");
    appendBackLine("/line x0 y0 x1 y1 col /rect x y w h col /fillrect x y w h col");
    appendBackLine("/circle x y r col /fillcircle x y r col /icon name x y col /clear-gfx");
    return;
  }
  if (strcmp(input, "/demo") == 0) { runDemo(); return; }
  if (strcmp(input, "/palette") == 0) { printPalette(); return; }
  if (strcmp(input, "/clear") == 0) { clearBackBuffer(); appendBackLine("back buffer cleared"); return; }
  if (strcmp(input, "/clear-gfx") == 0) { clearGfxCmds(); appendBackLine("graphics cleared"); return; }
  if (strcmp(input, "/normal") == 0) { resetDrawState(); appendBackLine("normal draw state"); return; }
  if (strcmp(input, "/bold") == 0) { drawState.bold = !drawState.bold; appendBackLine(drawState.bold ? "bold on" : "bold off"); return; }
  if (strncmp(input, "/bold ", 6) == 0) { drawState.bold = atoi(input + 6) != 0; appendBackLine(drawState.bold ? "bold on" : "bold off"); return; }
  if (strncmp(input, "/fg ", 4) == 0) {
    int idx = parseColorArg(input + 4);
    if (idx >= 0) { drawState.fg = idx; appendBackLine("fg set"); }
    else appendBackLine("bad fg");
    return;
  }
  if (strncmp(input, "/bg ", 4) == 0) {
    int idx = parseColorArg(input + 4);
    if (idx >= 0) { drawState.bg = idx; appendBackLine("bg set"); }
    else appendBackLine("bad bg");
    return;
  }
  if (strcmp(input, "/spam") == 0) {
    for (int i = 0; i < 48; i++) {
      appendBackString("spam line "); appendBackUnsigned(i); appendBackString(": abcdefghijklmnopqrstuvwxyz 0123456789\n");
    }
    return;
  }
  if (strcmp(input, "/status") == 0) {
    appendBackString("rows="); appendBackUnsigned(backBuffer.count);
    appendBackString(" view="); appendBackUnsigned(backBuffer.viewportStart);
    appendBackString(" fg="); appendBackUnsigned(drawState.fg);
    appendBackString(" bg="); appendBackUnsigned(drawState.bg);
    appendBackString(" bold="); appendBackUnsigned(drawState.bold);
    appendBackString(" gfx="); appendBackUnsigned(gfxCmdCount);
    appendBackChar('\n');
    return;
  }

  int x0, y0, x1, y1, x, y, w, h, r, col;
  char iconName[24];
  if (sscanf(input, "/line %d %d %d %d %d", &x0, &y0, &x1, &y1, &col) == 5) { drawGfxLine(x0, y0, x1, y1, col); appendBackLine("line ok"); return; }
  if (sscanf(input, "/fillrect %d %d %d %d %d", &x, &y, &w, &h, &col) == 5) { drawGfxFillRect(x, y, w, h, col); appendBackLine("fillrect ok"); return; }
  if (sscanf(input, "/rect %d %d %d %d %d", &x, &y, &w, &h, &col) == 5) { drawGfxRect(x, y, w, h, col); appendBackLine("rect ok"); return; }
  if (sscanf(input, "/fillcircle %d %d %d %d", &x, &y, &r, &col) == 4) { drawGfxFillCircle(x, y, r, col); appendBackLine("fillcircle ok"); return; }
  if (sscanf(input, "/circle %d %d %d %d", &x, &y, &r, &col) == 4) { drawGfxCircle(x, y, r, col); appendBackLine("circle ok"); return; }
  if (sscanf(input, "/icon %23s %d %d %d", iconName, &x, &y, &col) == 4) { drawIconCommand(iconName, x, y, col); appendBackLine("icon ok"); return; }

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
// Keyboard
// -----------------------------------------------------------------------------

void logKeyEvent(uint8_t key, PCKeyboard::KeyState state) {
  appendBackString("key "); appendBackHex8(key);
  appendBackString(" state "); appendBackUnsigned(static_cast<uint8_t>(state));
  appendBackChar('\n');
}

void processPrintableOrControl(uint8_t key) {
  if (key == KeyEsc) { appendBackLine("ESC pressed"); resetEditBuffer(); requestRender(); return; }
  if (key == '\r' || key == '\n') { commitEditBuffer(); requestRender(); return; }
  if (key == 8 || key == 0x7F) { backspaceEditChar(); requestRender(); return; }
  if (key == KeyCandidateLeft) { moveCursorLeft(); requestRender(); return; }
  if (key == KeyCandidateRight) { moveCursorRight(); requestRender(); return; }
  if (key == KeyCandidateDel) { deleteEditChar(); requestRender(); return; }
  if (key == KeyCandidateUp) {
    if (backBuffer.viewportStart > 0) { backBuffer.viewportStart--; backBuffer.followTail = false; }
    requestRender(); return;
  }
  if (key == KeyCandidateDown) {
    if (backBuffer.viewportStart + TranscriptRows < backBuffer.count) backBuffer.viewportStart++;
    else { backBuffer.followTail = true; scrollBackBufferToTail(); }
    requestRender(); return;
  }
  if (key >= 32 && key < 127) { insertEditChar(static_cast<char>(key)); requestRender(); return; }
  logKeyEvent(key, PCKeyboard::StatePress);
  requestRender();
}

void pollKeyboard() {
  while (pc_kbd.keyCount() > 0) {
    PCKeyboard::KeyEvent event = pc_kbd.keyEvent();
    uint8_t key = static_cast<uint8_t>(event.key);
    if (event.state == PCKeyboard::StatePress) processPrintableOrControl(key);
    else if (key != 0) { logKeyEvent(key, event.state); requestRender(); }
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
  resetDrawState();
  clearBackBuffer();
  resetEditBuffer();
  initDisplay();
  initKeyboard();
  appendBackLine("PicoCalc REPL primitive sketch");
  appendBackLine("/help for commands, /demo for visual test");
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
