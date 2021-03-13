#include <Wire.h>
#include "SparkFun_External_EEPROM.h"
#include <Utils.h>

#define SIZE_BYTES 128

ExternalEEPROM eeprom;

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(100);

  Wire.begin();
  if (!eeprom.begin(0x57)) {
    println("No EEPROM detected");
    while (true);
  }
  eeprom.setMemorySize(4096); // bytes
  println("Found %d bytes EEPROM", eeprom.length());

  println("Enter command> ");
}

void loop() {
  if (Serial.available() > 0) {
    println();
    String cmd = Serial.readString();
    cmd.trim();

    if (cmd == "d") {
      println("Dumping EEPROM");
      dump(SIZE_BYTES);
    }
    else if (cmd == "clear") {
      println("Clearing EEPROM");
      for (int i = 0; i < SIZE_BYTES; i++) {
        if (eeprom.read(i) != 0) {
          eeprom.write(i, 0);
        }
      }
    }
    else if (cmd == "test") {
      println("Writing test pattern");
      test(SIZE_BYTES);
    }

    println();
    println("Enter command> ");
  }
}

void test(int len) {
  for (int i = 0; i < len; i++) {
    eeprom.write(i, i % 4 + 1);
  }
}

void dump(int len) {
  for (int loc = 0; loc < len; loc += 4) {
    print("0x%04X :: ", loc);
    print("%02X ", eeprom.read(loc + 0));
    print("%02X ", eeprom.read(loc + 1));
    print("%02X ", eeprom.read(loc + 2));
    print("%02X ", eeprom.read(loc + 3));
    println();
  }
}

