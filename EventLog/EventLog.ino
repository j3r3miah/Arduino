#include <RTClib.h>
#include <Utils.h>
#include "EventLog.h"

RTC_DS3231 rtc;
DateTime now;

MemoryArray loggerStorage(256);
// EEPROMArray loggerStorage(0x57, 4096);
EventLog logger(loggerStorage);

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
  logger.write(now.unixtime(), EventType::BOOTED);

  println("Booted");
}

void loop() {
  static uint32_t lastTick;
  if (millis() - lastTick >= 1000) {
    lastTick = millis();
    now = rtc.now();
  }

  if (Serial.available() > 0) {
    String cmd = Serial.readString();
    cmd.trim();
    if (cmd == "logs") {
      println("\nEventLog\n---------");
      logger.doEach([&](uint32_t timestamp, uint8_t event) {
        println("%s :: %s",
          DateTime(timestamp).timestamp().c_str(),
          eventToString(event));
      });
      println();
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
    else if (cmd == "d") {
      println("Logging disconnected");
      logger.write(now.unixtime(), EventType::DISCONNECTED);
    }
  }
}

