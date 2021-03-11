#include <Wire.h>
#include "SparkFun_External_EEPROM.h"
#include <Utils.h>

ExternalEEPROM eeprom;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  if (!eeprom.begin(0x57)) {
    println("No EEPROM detected");
    while (true);
  }
  eeprom.setMemorySize(4096); // bytes
  println("Found %d bytes EEPROM", eeprom.length());

  uint32_t start = 0;
  uint32_t end = eeprom.length() - 1;
  eeprom.write(start, 13);
  eeprom.write(end, 31);
  println("%d = %d", start, eeprom.read(start));
  println("%d = %d", end, eeprom.read(end));
}

void loop() {

}
