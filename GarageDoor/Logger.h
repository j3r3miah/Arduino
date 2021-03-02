#include <RTClib.h>
#include <EEPROM.h>

#define RECORD_BYTES 5 // uint32_t timestamp + uint8_t flags
#define MAX_BYTES 256 // MEGA4809 has 256 bytes of EEPROM

enum EventType {
  BOOTED = 1 << 0,
  CONNECTED = 1 << 1,
  DISCONNECTED = 1 << 2,
  DOOR_OPENED = 1 << 3,
  DOOR_CLOSED = 1 << 4
};

typedef struct record_t {
  uint8_t events; // this should never be 255, the marker for empty memory
  uint32_t timestamp;
} Record;

class Logger {
private:
  const RTC_DS3231 rtc;
  uint8_t events;
  int nextIndex = -1;

  char buf[80];

  bool readLog(uint8_t index, Record& record) {
    int addr = index * RECORD_BYTES;
    uint8_t events;
    EEPROM.get(addr, events);
    if (events == 255) {
      // reached end of buffer
      return false;
    }

    record.events = events;
    EEPROM.get(addr + 1, record.timestamp);

    sprintf(buf, "Read @ %d: ", index);
    Serial.print(buf);
    print(record);
    return true;
  }

  void writeLog(uint8_t index, const Record& record) {
    int addr = index * RECORD_BYTES;

    sprintf(buf, "Write @ %d: ", index);
    Serial.print(buf);
    print(record);

    EEPROM.put(addr, record.events);
    EEPROM.put(addr + 1, record.timestamp);
  }

  void appendLog(const Record& record) {
    if (nextIndex == -1) {
      Record r;
      int i = 0;
      uint32_t maxTimestamp = 0;
      // find either the end of the buffer or the oldest entry
      // while (readLog(i, r)) {
      //   if (r.timestamp < maxTimestamp) break;
      //   maxTimestamp = r.timestamp;
      //   i++;
      // }
      nextIndex = i;
      Serial.print("Begin logging at ");
      Serial.println(nextIndex);
    }

    writeLog(nextIndex++, record);

    if (nextIndex > MAX_BYTES / RECORD_BYTES) {
      nextIndex = 0;
    }
  }

  void print(const Record& record) {
    Serial.println(record.timestamp);
    Serial.print(" :: ");
    Serial.print(record.events, BIN);
  }

public:
  Logger(const RTC_DS3231& rtc) : rtc(rtc) {}

  void log(EventType event) {
    // Serial.print("[logger] ");
    // Serial.println(event);
    events |= event;
  }

  // TODO call this once a second instead of every loop
  void update() {
    if (events != 0) {
      Record record;
      record.events = events;
      record.timestamp = rtc.now().unixtime();
      appendLog(record);
      events = 0;
    }
  }

  void printLogs() {
      Record r;
      int i = 0;
      uint32_t maxTimestamp = 0;
      // find either the end of the buffer or the oldest entry
      while (readLog(i++, r)) {
        // if (r.timestamp < maxTimestamp) break;
        // maxTimestamp = r.timestamp;
      }
      // int startIndex = r.events == 255 ? 0 : i;
      // for (i = 0; i < MAX_BYTES / RECORD_BYTES; i++) {
      // }
  }
};
