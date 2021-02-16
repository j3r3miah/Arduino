#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);

byte backslash[8] = {
  0b00000,
  0b10000,
  0b01000,
  0b00100,
  0b00010,
  0b00001,
  0b00000,
  0b00000
};

byte pipe[8] = {
  0b00000,
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b00000
};


void setup()
{
  lcd.init();
  lcd.createChar(0, pipe);
  lcd.createChar(3, backslash);
  lcd.backlight();
  lcd.setCursor(4, 1);
  lcd.print("Hello world! \\");

  Serial.begin(9600);
}

void tickSpinner(int i, int j, int val)
{
    lcd.setCursor(i, j);
    switch (val) {
      case 0:
        lcd.write(byte(0)); // pipe
        break;
      case 1:
        lcd.print("/");
        break;
      case 2:
        lcd.print("-");
        break;
      case 3:
        lcd.write(byte(3)); // backslash
        break;
    }
}

static int tick = 0;

void loop()
{
  tickSpinner(9, 2, tick);
  if (++tick > 3) tick = 0;
  delay(200);
}
