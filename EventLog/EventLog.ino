#include <RTClib.h>
#include <Utils.h>
#include "EventLog.h"

RTC_DS3231 rtc;
DateTime now;

// MemoryArray storage(256);
// EEPROMArray storage(0x57, 64); // actually 4096 bytes
FRAMArray storage(0x50, 32768); // actually 32kb
EventLog logger(storage);

enum EventType : uint8_t {
  BOOTED,
  CONNECTED,
  DISCONNECTED,
  DOOR_OPENED,
  DOOR_CLOSED
};

static const char* eventToString(uint8_t event) {
  switch ((EventType)event) {
    case BOOTED: return "Booted";
    case CONNECTED: return "Connected";
    case DISCONNECTED: return "Disconnected";
    case DOOR_OPENED: return "Opened";
    case DOOR_CLOSED: return "Closed";
    default: return "Unknown";
  }
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(100);

  if (!rtc.begin()) {
    println("Communication with RTC module failed");
    while (true);
  }

  now = rtc.now();
  logger.init();
}

void loop() {
  static uint32_t lastTick;
  if (millis() - lastTick >= 1000) {
    lastTick = millis();
    now = rtc.now();
  }

  if (Serial.available() > 0) {
    println();
    String cmd = Serial.readString();
    cmd.trim();
    if (cmd == "p") {
      println("--- EventLog (%d) ---", storage.len());
      int i = 0;
      logger.doEach([&](uint32_t timestamp, uint8_t event) {
        println("%02d: %s :: %s", i++,
          DateTime(timestamp).timestamp().c_str(),
          eventToString(event));
      });
    }
    else if (cmd == "r") {
      println("--- EventLog (%d) ---", storage.len());
      int i = 0;
      logger.doEach([&](uint32_t timestamp, uint8_t event) {
        println("%02d: %s :: %s", i++,
          DateTime(timestamp).timestamp().c_str(),
          eventToString(event));
      }, true);
    }
    else if (cmd == "c") {
      println("Logging door closed");
      logger.write(now.unixtime(), EventType::DOOR_CLOSED);
    }
    else if (cmd == "o") {
      println("Logging door opened");
      logger.write(now.unixtime(), EventType::DOOR_OPENED);
    }
    else if (cmd == "w") {
      println("Logging connected");
      logger.write(now.unixtime(), EventType::CONNECTED);
    }
    else if (cmd == "clear") {
      println("Clearing EEPROM");
      storage.clear();
    }
    else if (cmd == "d") {
      println("Dumping EEPROM");
      storage.dump();
    }
    else if (cmd == "r") {
      println("Resetting EEPROM pointers");
      storage.resetPointers();
    }
  }
}

