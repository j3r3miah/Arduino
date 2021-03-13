#include <functional>
#include <RTClib.h>
#include <SparkFun_External_EEPROM.h>
#include <Utils.h>

struct Record {
  uint32_t timestamp;
  uint8_t event;
};

class IStorage {
protected:
  int head = -1;
  int tail = -1;

  virtual const Record get(int index) = 0;
  virtual void put(int index, Record record) = 0;

  void increment(int &ptr) {
    if (++ptr >= size())
      ptr = 0;
  }

public:
  virtual void init() = 0;

  virtual int size() = 0;

  int len() {
    if (empty()) return 0;
    if (tail == 0) return head + 1;
    return size();
  }

  bool empty() {
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

  const Record last() {
    return get(head);
  }

  void doEach(const std::function<void (uint32_t, uint8_t)>& f) {
    if (empty()) return;
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

  const Record last() {
    return storage.last();
  }

  bool empty() {
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

  void init() { }

  int size() {
    return d_size;
  }

  const Record get(int index) {
    return arr[index];
  }

  void put(int index, Record record) {
    arr[index] = record;
  }
};

class EEPROMArray : public IStorage {
  const int RECORD_SIZE = 5;

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
    resetPointers();
  }

  int size() {
    return sizeBytes / RECORD_SIZE;
  }

  const Record get(int index) {
    Record record = {
      readUInt(index * RECORD_SIZE),
      readByte(index * RECORD_SIZE + 4)
    };
    return record;
  }

  void put(int index, Record record) {
    write(index * RECORD_SIZE, record.timestamp);
    write(index * RECORD_SIZE + 4, record.event);
  }

  void clear() {
    for (int i = 0; i < sizeBytes; i++) {
      if (readByte(i) != 0) {
        write(i, (uint8_t)0);
      }
    }
    resetPointers();
  }

  void resetPointers() {
    uint32_t maxTime = 0;
    int maxIndex = -1;
    for (int i = 0; i < size(); i++) {
      uint32_t timestamp = readUInt(i * RECORD_SIZE);
      if (timestamp == 0) break;
      if (timestamp >= maxTime) {
        maxTime = timestamp;
        maxIndex = i;
      }
      else {
        break;
      }
    }
    head = tail = maxIndex;

    // buffer is empty
    if (head == -1) return;
    // set tail one after head, assuming circle buffer is full
    increment(tail);
    // if tail timestamp is zero, circle buffer is not actually full
    uint32_t tailTime = 1;
    if (head < size() - 1) {
      tailTime = readUInt(tail * RECORD_SIZE);
    }
    if (tailTime == 0) {
      tail = 0;
    }
    // println("head=%d, tail=%d", head, tail);
  }

  void dump() {
    int len = size() * RECORD_SIZE;
    println("head=%d, tail=%d", head, tail);
    for (int loc = 0; loc < len; loc += 5) {
      print("0x%04X - ", loc);
      print("%02X ", eeprom.read(loc + 0));
      print("%02X ", eeprom.read(loc + 1));
      print("%02X ", eeprom.read(loc + 2));
      print("%02X ", eeprom.read(loc + 3));
      print("%02X - ", eeprom.read(loc + 4));
      Record record = {
        readUInt(loc),
        readByte(loc + 4)
      };
      println("%s :: %d", DateTime(record.timestamp).timestamp().c_str(), record.event);
    }
  }

private:
  void write(int location, uint32_t value) {
    char *p = (char *)&value;
    for (int i = 0; i < 4; i++)
      eeprom.write(location + i, p[i]);
  }

  void write(int location, uint8_t value) {
    eeprom.write(location, value);
  }

  uint8_t readByte(int location) {
    return eeprom.read(location);
  }

  uint32_t readUInt(int location) {
    uint32_t value;
    char *p = (char *)&value;
    for (int i = 0; i < 4; i++)
      p[i] = eeprom.read(location + i);
    return value;
  }
};
