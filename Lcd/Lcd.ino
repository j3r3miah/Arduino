#include <Lcd.h>

Lcd lcd;

void setup() {
  lcd.init();
  lcd.backlight(true);
}

void loop() {
  updateDisplay();
  lcd.update();
}

void updateDisplay() {
  static uint32_t lastUpdate;

  if (millis() - lastUpdate >= 1000) {
    lastUpdate = millis();
    lcd.printf(2, "qwerty");
  }
}
