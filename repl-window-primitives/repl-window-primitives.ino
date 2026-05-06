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
//   - full-screen renderer: transcript + status + input line

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

// -----------------------------------------------------------------------------
// Edit buffer: mutable active input line
// -----------------------------------------------------------------------------

struct EditBufferState {
  char text[InputBufferSize];
  uint16_t len = 0;
  uint16_t cursor = 0;
};

EditBufferState editBuffer;

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

void commitEditBuffer() {
  appendBackString("> ");
  for (uint16_t i = 0; i < editBuffer.len; i++) appendBackChar(editBuffer.text[i]);
  appendBackChar('\n');
  appendBackString("echo: ");
  for (uint16_t i = 0; i < editBuffer.len; i++) appendBackChar(editBuffer.text[i]);
  appendBackChar('\n');
  resetEditBuffer();
}

// -----------------------------------------------------------------------------
// Renderer
// -----------------------------------------------------------------------------

void drawCell(int row, int col, char ch, uint16_t fg = TFT_WHITE, uint16_t bg = TFT_BLACK) {
  const int x = col * CharWidth;
  const int y = row * Leading;
  tft.drawChar(x, y, ch, fg, bg, 1);
}

void clearScreenRow(int row, uint16_t bg = TFT_BLACK) {
  tft.fillRect(0, row * Leading, ScreenWidth, Leading, bg);
}

void drawTextClipped(int row, int col, const char *text, uint16_t fg = TFT_WHITE, uint16_t bg = TFT_BLACK) {
  while (*text && col < Columns) {
    drawCell(row, col++, *text++, fg, bg);
  }
}

void renderTranscript() {
  for (int row = 0; row < TranscriptRows; row++) {
    clearScreenRow(row);
    const uint16_t logical = backBuffer.viewportStart + row;
    if (logical >= backBuffer.count) continue;
    const uint16_t physical = physicalRowForLogical(logical);
    for (int col = 0; col < Columns; col++) {
      const char ch = backBuffer.rows[physical][col];
      if (ch != ' ') drawCell(row, col, ch);
    }
  }
}

void renderStatus() {
  const int row = TranscriptRows;
  clearScreenRow(row, TFT_DARKGREY);
  char status[80];
  snprintf(status, sizeof(status), "rows %u/%u view %u cursor %u len %u",
           backBuffer.count,
           BackBufferRows,
           backBuffer.viewportStart,
           editBuffer.cursor,
           editBuffer.len);
  drawTextClipped(row, 0, status, TFT_BLACK, TFT_DARKGREY);
}

void renderInput() {
  const int firstRow = TranscriptRows + StatusRows;
  for (int row = firstRow; row < Lines; row++) clearScreenRow(row);

  drawTextClipped(firstRow, 0, "> ", TFT_GREEN, TFT_BLACK);

  int screenRow = firstRow;
  int screenCol = 2;
  for (uint16_t i = 0; i < editBuffer.len && screenRow < Lines; i++) {
    if (screenCol >= Columns) {
      screenCol = 0;
      screenRow++;
      if (screenRow >= Lines) break;
    }
    drawCell(screenRow, screenCol, editBuffer.text[i], TFT_WHITE, TFT_BLACK);
    screenCol++;
  }

  uint16_t cursorVisual = editBuffer.cursor + 2;
  int cursorRow = firstRow + (cursorVisual / Columns);
  int cursorCol = cursorVisual % Columns;
  if (cursorRow < Lines) {
    char underCursor = ' ';
    if (editBuffer.cursor < editBuffer.len) underCursor = editBuffer.text[editBuffer.cursor];
    drawCell(cursorRow, cursorCol, underCursor == ' ' ? '_' : underCursor, TFT_BLACK, TFT_GREEN);
  }
}

void renderAll() {
  renderTranscript();
  renderStatus();
  renderInput();
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
    return;
  }

  if (key == '\r' || key == '\n') {
    commitEditBuffer();
    return;
  }

  if (key == 8 || key == 0x7F) {
    backspaceEditChar();
    return;
  }

  // Candidate mappings from existing uLisp filtering. Confirm on hardware.
  if (key == KeyCandidateLeft) {
    moveCursorLeft();
    appendBackLine("candidate LEFT");
    return;
  }
  if (key == KeyCandidateRight) {
    moveCursorRight();
    appendBackLine("candidate RIGHT");
    return;
  }
  if (key == KeyCandidateDel) {
    deleteEditChar();
    appendBackLine("candidate DELETE");
    return;
  }
  if (key == KeyCandidateUp) {
    if (backBuffer.viewportStart > 0) {
      backBuffer.viewportStart--;
      backBuffer.followTail = false;
    }
    appendBackLine("candidate UP / scroll up");
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
    return;
  }

  if (key >= 32 && key < 127) {
    insertEditChar(static_cast<char>(key));
    return;
  }

  logKeyEvent(key, PCKeyboard::StatePress);
}

void pollKeyboard() {
  while (pc_kbd.keyCount() > 0) {
    const PCKeyboard::KeyEvent event = pc_kbd.keyEvent();
    const uint8_t key = static_cast<uint8_t>(event.key);
    if (event.state == PCKeyboard::StatePress) {
      processPrintableOrControl(key);
    } else if (key != 0) {
      logKeyEvent(key, event.state);
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

  for (int row = 0; row < BackBufferRows; row++) clearBackRow(row);
  resetEditBuffer();

  initDisplay();
  initKeyboard();

  appendBackLine("PicoCalc REPL primitive sketch");
  appendBackLine("Type text. Enter commits. Arrows are logged/tested.");
  appendBackLine("Goal: validate display + keyboard before uLisp integration.");
  renderAll();
}

void loop() {
  pollKeyboard();
  renderAll();
  delay(20);
}
