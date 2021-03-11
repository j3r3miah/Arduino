#include <functional>
#include <RTClib.h>
#include <Utils.h>

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

class IArray {
public:
  virtual int size() const = 0;
  virtual const Record get(int index) const = 0;
  virtual void put(int index, Record record) = 0;
};

class EventLog {
  IArray& storage;
  int head = -1;
  int tail = -1;

public:
  EventLog(IArray& storage) : storage(storage) {}

  void write(uint32_t timestamp, EventType event) {
    increment(head);
    if (tail == head)
      increment(tail);
    if (tail == -1)
      tail = 0;

    Record r = { timestamp, event };
    storage.put(head, r);
  }

  void doEach(const std::function<void (uint32_t, EventType)>& f) {
    int p = tail;
    while (true) {
      Record r = storage.get(p);
      f(r.timestamp, r.event);
      if (p == head) break;
      increment(p);
    }
  }

  const Record last() const {
    return storage.get(head);
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
    if (++ptr >= storage.size())
      ptr = 0;
  }
};

class MemoryArray : public IArray {
  int d_size;
  Record *arr;

public:
  MemoryArray(int size) : d_size(size) {
    arr = new Record[size]();
  }

  int size() const {
    return d_size;
  }

  const Record get(int index) const {
    return arr[index];
  }

  void put(int index, Record record) {
    arr[index] = record;
  }
};
