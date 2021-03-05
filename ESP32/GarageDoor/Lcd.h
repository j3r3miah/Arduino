#include <stdarg.h>
#include <Print.h>
#include <LiquidCrystal_I2C.h>

#define ROWS 4
#define COLS 20

class Lcd {
private:
  LiquidCrystal_I2C lcd;
  unsigned long sleepAfterMillis = 0;
  unsigned long activatedMillis = 0;
  bool backlightEnabled = false;
  char buf[COLS] = {0};
  char buf2[COLS] = {0};

public:
  Lcd() : lcd(LiquidCrystal_I2C(0x27, COLS, ROWS)) { }

  void init() {
    lcd.init();
  }

  void update() {
    if (sleepAfterMillis > 0
        && activatedMillis > 0
        && millis() - activatedMillis > sleepAfterMillis) {
      backlight(false);
    }
  }

  void sleepAfter(unsigned long millis) {
    sleepAfterMillis = millis;
  }

  void backlight(bool enable) {
    backlightEnabled = enable;
    lcd.setBacklight(enable);
    activatedMillis = enable ? millis() : 0;
  }

  void toggleBacklight() {
    backlight(!backlightEnabled);
  }

  void printf(int row, int col, const char *fmt, ...) {
    va_list args;
    va_start (args, fmt );
    vprintf(row, col, fmt, args);
    va_end (args);
  }

  void printf(int row, const char *fmt, ...) {
    va_list args;
    va_start (args, fmt );
    vprintf(row, 0, fmt, args);
    va_end (args);
  }

  void vprintf(int row, int col, const char *fmt, va_list args) {
    vsnprintf(buf, sizeof(buf), fmt, args);
    // pad with spaces to end of line
    snprintf(buf2, sizeof(buf2), "%-*s", COLS - col, buf);
    lcd.setCursor(col, row);
    lcd.print(buf2);
  }

  void print(int row, String msg) {
    print(row, 0, msg);
  }

  void print(int row, int col, String msg) {
    // pad with spaces to end of line
    snprintf(buf2, sizeof(buf2), "%-*s", COLS - col, msg.c_str());
    lcd.setCursor(col, row);
    lcd.print(buf2);
  }

  void clear(int row) {
    snprintf(buf2, sizeof(buf2), "%-*s", COLS, "");
    lcd.setCursor(0, row);
    lcd.print(buf2);
  }

  Print *getPrinter(int col, int row) {
    lcd.setCursor(col, row);
    return &lcd;
  }
};
