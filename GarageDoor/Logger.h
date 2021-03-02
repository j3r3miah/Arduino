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

  char buf[80];

  template <class T> int write(int addr, const T& value) {
    const byte* p = (const byte*)(const void*)&value;
    unsigned int i;
    for (i = 0; i < sizeof(value); i++)
      EEPROM.write(addr++, *p++);
    return i;
  }

  template <class T> int read(int addr, T& value) {
    byte* p = (byte*)(void*)&value;
    unsigned int i;
    for (i = 0; i < sizeof(value); i++)
      *p++ = EEPROM.read(addr++);
    return i;
  }

  void writeLog(uint8_t index, const Record& record) {
    int addr = index * RECORD_BYTES;

    sprintf(buf, "Write @ %d: ", addr);
    Serial.print(buf);
    print(record);

    write(addr, record.events);
    write(addr + 1, record.timestamp);
  }

  void readLog(uint8_t index, const Record& record) {
    int addr = index * RECORD_BYTES;
    read(addr, record.events);
    read(addr + 1, record.timestamp);

    sprintf(buf, "Read @ %d: ", addr);
    Serial.print(buf);
    print(record);
  }

  void print(const Record& record) {
    Serial.print(record.events, BIN);
    Serial.print(" :: ");
    Serial.println(record.timestamp);
  }

public:
  Logger(const RTC_DS3231& rtc) : rtc(rtc) {}

#ifdef XXX
  void log(EventType event) {
    Serial.print("[logger] ");
    Serial.println(event);
    events |= event;
  }

  int curIndex = 0;

  // TODO call this once a second instead of every loop
  void update() {
    if (events != 0) {
      Record record;
      record.events = events;
      record.timestamp = rtc.now().unixtime();
      writeLog(curIndex++, record);
      events = 0;
    }
  }

  void printLogs() {
    // for (int i = 0; i < MAX_BYTES / RECORD_BYTES; i++) {
    for (int i = 0; i < 5; i++) {
      int addr = i * RECORD_BYTES;

      uint8_t b;
      read(addr, b);
      sprintf(buf, "Byte %d = ", addr);
      Serial.print(buf);
      Serial.println(b, BIN);
      // if (b == 255) {
      //   // reached unwritten memory
      //   break;
      // }

      Record record;
      readLog(i, record);
    }
  }
#else
  void log(EventType event) { }
  void update() { }
  void printLogs() {
  }
#endif
};
