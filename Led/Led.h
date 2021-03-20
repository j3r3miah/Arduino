class Led {
private:
  const int DELAY_MILLIS = 50;
  const int pin;

  unsigned int blinksToGo = 0;
  unsigned long lastBlinkMillis;

public:
  Led(int pin) : pin(pin) { }

  void init() {
    init(0);
  }

  void init(unsigned int blinks) {
    pinMode(pin, OUTPUT);
    blink(blinks);
  }

  void update() {
    unsigned long now = millis();
    if (blinksToGo > 0 && now - lastBlinkMillis > DELAY_MILLIS) {
      blinksToGo--;
      lastBlinkMillis = now;
      toggle();
    }
  }

  void set(bool enable) {
    digitalWrite(pin, enable ? HIGH : LOW);
  }

  void on() {
    digitalWrite(pin, HIGH);
  }

  void off() {
    digitalWrite(pin, LOW);
  }

  void toggle() {
    digitalWrite(pin, !digitalRead(pin));
  }

  void blink(unsigned int times) {
    blinksToGo = times * 2;
    lastBlinkMillis = 0;
    off();
  }
};
