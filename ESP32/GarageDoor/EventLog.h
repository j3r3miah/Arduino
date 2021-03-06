#include <functional>
#include <RTClib.h>
#include "Utils.h"

#define LOG_SIZE 4

enum EventType {
  BOOTED,
  CONNECTED,
  DISCONNECTED,
  DOOR_OPENED,
  DOOR_CLOSED
};

struct Record {
  uint32_t timestamp;
  EventType event;
};

class EventLog {
  Record logs[LOG_SIZE] = {};
  int head = -1;
  int tail = -1;

public:
  void write(uint32_t timestamp, EventType event) {
    increment(head);
    if (tail == head)
      increment(tail);
    if (tail == -1)
      tail = 0;

    // println("head=%d, tail=%d", head, tail);
    logs[head].timestamp = timestamp;
    logs[head].event = event;
    // print();
  }

  void print() {
    println("--- Log ---");
    int p = tail;
    while (true) {
      DateTime dt(logs[p].timestamp);
      println("%s :: %s", dt.timestamp().c_str(), toString(logs[p].event));
      if (p == head) break;
      increment(p);
    }
  }

  void doEach(const std::function<void (uint32_t, EventType)>& f) {
    int p = tail;
    while (true) {
      f(logs[p].timestamp, logs[p].event);
      if (p == head) break;
      increment(p);
    }
  }

  static const char* toString(EventType event) {
    switch (event) {
      case BOOTED: return "System booted";
      case CONNECTED: return "WiFi connected";
      case DISCONNECTED: return "WiFi disconnected";
      case DOOR_OPENED: return "Door opened";
      case DOOR_CLOSED: return "Door closed";
      default: return "Unknown event";
    }
  }

private:
  void increment(int &ptr) {
    if (++ptr >= LOG_SIZE)
      ptr = 0;
  }
};
