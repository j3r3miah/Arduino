#include <Bounce2.h>
#include <RTClib.h>
#include <TimerEvent.h>
#include <WiFi.h>

#include "Lcd.h"
#include "Led.h"
#include "Secrets.h"
#include "Utils.h"

#define LED_PIN LED_BUILTIN
#define REED_SWITCH_PIN 27

char DOW[7][12] = {
  "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
};

char ssid[] = SECRET_SSID;
char password[] = SECRET_PASS;

Lcd lcd;
RTC_DS3231 rtc;
Led led(LED_PIN);
Bounce reedSwitch;

TimerEvent timer;
wl_status_t wifiStatus;

void setup() {
  Serial.begin(115200);
  led.init(5);
  setupRTC();
  lcd.init();
  lcd.backlight(true);
  timer.set(1000, updateDisplay);
  updateDisplay();
  reedSwitch.attach(REED_SWITCH_PIN, INPUT_PULLUP);
  reedSwitch.interval(25);
  WiFi.begin(ssid, password);
}

void loop() {
  led.update();
  lcd.update();
  timer.update();
  reedSwitch.update();

  if (reedSwitch.fell()) {
      // sendPush("Garage Door", "Door has closed");
      lcd.backlight(true);
      led.blink(2);
  }
  else if (reedSwitch.rose()) {
      // sendPush("Garage Door", "Door has opened");
      lcd.backlight(true);
      led.blink(2);
  }
}

void setupRTC() {
  if (!rtc.begin()) {
    println("Communication with RTC module failed");
    while (true);
  }
  if (rtc.lostPower()) {
    println("RTC lost power; setting time");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}

void updateDisplay() { // called once per second
  DateTime now = rtc.now();
  lcd.printf(3, "%s %02d:%02d:%02d",
             DOW[now.dayOfTheWeek()],
             now.hour(), now.minute(), now.second());

  wl_status_t status = WiFi.status();
  if (status != wifiStatus) {
    lcd.print(0, wifiStatusString(status));
    lcd.print(1, WiFi.SSID());
    if (status == WL_CONNECTED) {
      println("Connected to %s: %s",
              WiFi.SSID().c_str(),
              WiFi.localIP().toString().c_str());
      lcd.print(2, WiFi.localIP().toString());
    }
    else {
      println("WiFi status: %s", wifiStatusString(status));
      lcd.clear(1); // clear ssid
      lcd.clear(2); // clear ip
    }
    wifiStatus = status;
  }

  if (WiFi.status() == WL_CONNECTED) {
    lcd.printf(0, 13, "%ld dBm", WiFi.RSSI());
  }
}
