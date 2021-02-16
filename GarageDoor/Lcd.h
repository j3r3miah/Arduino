#include <Print.h>
#include <LiquidCrystal_I2C.h>

class Lcd {
private:
  LiquidCrystal_I2C lcd;
  unsigned long sleepAfterMillis = 0;
  unsigned long activatedMillis = 0;
  bool backlightEnabled = false;

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
    clear(row);
    lcd.setCursor(0, row);
    lcd.print(msg);
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
