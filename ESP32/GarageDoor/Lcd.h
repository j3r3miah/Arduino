#include <stdarg.h>
#include <Print.h>
#include <LiquidCrystal_I2C.h>

class Lcd {
private:
  LiquidCrystal_I2C lcd;
  unsigned long sleepAfterMillis = 0;
  unsigned long activatedMillis = 0;
  bool backlightEnabled = false;
  char buf[20] = {0};
  char buf2[20] = {0};

public:
  Lcd() : lcd(LiquidCrystal_I2C(0x27, 20, 4)) { }

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

  void print(int row, String msg) {
    sprintf(buf, "%-20s", msg.c_str());
    lcd.setCursor(0, row);
    lcd.print(buf);
  }

  void print(int row, int col, String msg) {
    lcd.setCursor(col, row);
    lcd.print(msg);
  }

  void printf(int row, int col, char *fmt, ...) {
    va_list args;
    va_start (args, fmt );
    vprintf(row, col, fmt, args);
    va_end (args);
  }

  void printf(int row, char *fmt, ...) {
    va_list args;
    va_start (args, fmt );
    vprintf(row, 0, fmt, args);
    va_end (args);
  }

  void vprintf(int row, int col, char *fmt, va_list args) {
    vsnprintf(buf, sizeof(buf), fmt, args);
    // pad to 20 chars (clear to end of line)
    snprintf(buf2, sizeof(buf2), "%-20s", buf);
    lcd.setCursor(col, row);
    lcd.print(buf2);
  }

  void clear(int row) {
    lcd.setCursor(0, row);
    lcd.print("                    ");
  }

  void backlight(bool enable) {
    backlightEnabled = enable;
    lcd.setBacklight(enable);
    activatedMillis = enable ? millis() : 0;
  }

  void toggleBacklight() {
    backlight(!backlightEnabled);
  }

  Print *getPrinter(int col, int row) {
    lcd.setCursor(col, row);
    return &lcd;
  }
};
