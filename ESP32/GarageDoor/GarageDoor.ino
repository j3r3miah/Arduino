#include <WiFi.h>
#include <TimerEvent.h>
#include <RTClib.h>

#include "Utils.h"
#include "Lcd.h"
#include "Secrets.h"

char DOW[7][12] = {
  "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
};

char ssid[] = SECRET_SSID;
char password[] = SECRET_PASS;

Lcd lcd;
RTC_DS3231 rtc;

TimerEvent timer;
wl_status_t wifiStatus;
char buf[80];

void setup() {
  Serial.begin(115200);
  setupRTC();
  lcd.init();
  lcd.backlight(true);
  timer.set(1000, updateDisplay);
  updateDisplay();
  WiFi.begin(ssid, password);
}

void loop() {
  lcd.update();
  timer.update();
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

// called once per second
void updateDisplay() {
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
    sprintf(buf, "%ld dBm", WiFi.RSSI());
    lcd.print(0, 13, buf);
  }
}

const char *wifiStatusString(wl_status_t status) {
  switch (status) {
    case WL_NO_SHIELD: return "No shield";
    case WL_IDLE_STATUS: return "Idle";
    case WL_NO_SSID_AVAIL: return "Searching...";
    case WL_SCAN_COMPLETED: return "Scan Completed";
    case WL_CONNECTED: return "Connected";
    case WL_CONNECT_FAILED: return "Connect Failed";
    case WL_CONNECTION_LOST: return "Connection Lost";
    case WL_DISCONNECTED: return "Disconnected";
    default: return "Unknown Error";
  }
}
