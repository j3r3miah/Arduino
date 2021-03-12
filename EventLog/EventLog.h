#include <functional>
#include <RTClib.h>
#include <SparkFun_External_EEPROM.h>
#include <Utils.h>

// TODO
//   the storage class needs to manage the head/tail pointers
//   upon init, the eeprom storage needs to figure out previous head/tail values
//   logging to i2c causes issues with interrupts

struct Record {
  uint32_t timestamp;
  uint8_t event;
};

class IStorage {
protected:
  int head = -1;
  int tail = -1;

  virtual const Record get(int index) const = 0;
  virtual void put(int index, Record record) = 0;

  void increment(int &ptr) {
    if (++ptr >= size())
      ptr = 0;
  }

public:
  virtual void init() = 0;

  virtual int size() const = 0;

  bool empty() const {
    return head == -1;
  }

  void add(Record record) {
    increment(head);
    if (tail == head)
      increment(tail);
    if (tail == -1)
      tail = 0;

    put(head, record);
  }

  const Record last() const {
    return get(head);
  }

  void doEach(const std::function<void (uint32_t, uint8_t)>& f) {
    int p = tail;
    while (true) {
      Record r = get(p);
      f(r.timestamp, r.event);
      if (p == head) break;
      increment(p);
    }
  }
};

class EventLog {
  IStorage& storage;

public:
  EventLog(IStorage& storage) : storage(storage) {}

  void init() {
    storage.init();
  }

  void write(uint32_t timestamp, uint8_t event) {
    Record r = { timestamp, event };
    storage.add(r);
  }

  void doEach(const std::function<void (uint32_t, uint8_t)>& f) {
    storage.doEach(f);
  }

  const Record last() const {
    return storage.last();
  }

  bool empty() const {
    return storage.empty();
  }
};

class MemoryArray : public IStorage {
  int d_size;
  Record *arr;

public:
  MemoryArray(int size) : d_size(size) {
    arr = new Record[size]();
  }

  void init() {
    Record r1 = { 1615559784, 3 };
    Record r2 = { 1615559784, 2 };
    put(0, r1);
    put(1, r2);
    head = 1;
    tail = 0;
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

/*
class EEPROMArray : public IStorage {
  uint8_t i2cAddress;
  uint32_t sizeBytes;
  ExternalEEPROM eeprom;

public:
  EEPROMArray(uint8_t i2cAddress, uint32_t sizeBytes) :
    i2cAddress(i2cAddress), sizeBytes(sizeBytes) {}

  void init() {
    if (!eeprom.begin(i2cAddress)) {
      println("No EEPROM found");
      while (true);
    }
    eeprom.setMemorySize(sizeBytes);
  }

  int size() {
    return sizeBytes;
  }

  const Record get(int index) {
    Record record;
    eeprom.get(index, record);
    return record;
  }

  void put(int index, Record record) {
    eeprom.put(index, record);
  }
};
*/
