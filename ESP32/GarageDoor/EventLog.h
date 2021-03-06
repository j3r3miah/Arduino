#include <functional>
#include <RTClib.h>
#include "Utils.h"

#define LOG_SIZE 256

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

    logs[head].timestamp = timestamp;
    logs[head].event = event;
  }

  void doEach(const std::function<void (uint32_t, EventType)>& f) {
    int p = tail;
    while (true) {
      f(logs[p].timestamp, logs[p].event);
      if (p == head) break;
      increment(p);
    }
  }

  const Record last() const {
    return logs[head];
  }

  bool empty() const {
    return head == -1;
  }

  static const char* toString(EventType event) {
    switch (event) {
      case BOOTED: return "Booted";
      case CONNECTED: return "Connected";
      case DISCONNECTED: return "Disconnected";
      case DOOR_OPENED: return "Opened";
      case DOOR_CLOSED: return "Closed";
      default: return "Unknown";
    }
  }

private:
  void increment(int &ptr) {
    if (++ptr >= LOG_SIZE)
      ptr = 0;
  }
};
