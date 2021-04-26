// disable opener, push notifications, and non-volatile logging
// #define DEV_MODE

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <Pushsafer.h>
#include <Bounce2.h>
#include <RTClib.h>

#include <EventLog.h>
#include <WiFiUtils.h>
#include <Utils.h>
#include <Lcd.h>
#include <Led.h>

#include "Secrets.h"

#define MDNS_NAME "garage"
#define LED_PIN LED_BUILTIN
#define REED_SWITCH_PIN 26
#define OPENER_PIN 32
#define TOUCH_PIN 33
#define TOUCH_THRESHOLD 20

char DOW[7][12] = {
  "Sunday",
  "Monday",
  "Tuesday",
  "Wednesday",
  "Thursday",
  "Friday",
  "Saturday"
};

enum EventType : uint8_t {
  BOOTED,
  CONNECTED,
  DISCONNECTED,
  DOOR_OPENED,
  DOOR_CLOSED
};

char ssid[] = WIFI_SSID;
char password[] = WIFI_PASS;
char pusherKey[] = PUSHER_API_KEY;

Lcd lcd;
RTC_DS3231 rtc;
Led led(LED_PIN);
Bounce reedSwitch;
AsyncWebServer server(80);
WiFiClient pushClient;
Pushsafer pusher(pusherKey, pushClient);
FRAMArray storage(0x50, 32768);
EventLog logger(storage);
String recentLogs;
DateTime now;

void setup() {
  Serial.begin(115200);
  setupClock();
  now = rtc.now();
  logger.init();
  log(EventType::BOOTED);

  led.init(5);
  lcd.init();
  lcd.sleepAfter(1000 * 60 * 3);
  lcd.backlight(true);

  pinMode(TOUCH_PIN, INPUT);
  pinMode(OPENER_PIN, OUTPUT);
  reedSwitch.attach(REED_SWITCH_PIN, INPUT_PULLUP);
  reedSwitch.interval(25);

  setupNetwork();
  setupServer();
}

void loop() {
  led.update();
  lcd.update();
  updateClock();
  updateDisplay();
  checkReedSwitch();
  checkTouchSwitch();
}

void setupClock() {
  if (!rtc.begin()) {
    println("Communication with RTC module failed");
    while (true);
  }
  if (rtc.lostPower()) {
    println("RTC lost power; setting time");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}

void setupNetwork() {
  WiFi.disconnect(true);

  WiFi.onEvent([](WiFiEvent_t event) {
    switch (event) {
      case SYSTEM_EVENT_STA_CONNECTED:
        log(EventType::CONNECTED);
        // enable ipv6 to make mDNS resolution faster for clients
        WiFi.enableIpV6();
        break;

      // case SYSTEM_EVENT_STA_GOT_IP:
      case SYSTEM_EVENT_AP_STA_GOT_IP6:
        println("Connected to %s: %s / %s",
                WiFi.SSID().c_str(),
                WiFi.localIP().toString().c_str(),
                WiFi.localIPv6().toString().c_str());
        if (!MDNS.begin(MDNS_NAME)) {
          println("mDNS responder failed");
        } else {
          println("mDNS responder started");
          MDNS.addService("http", "tcp", 80);
        }
        break;

      case SYSTEM_EVENT_STA_DISCONNECTED:
        log(EventType::DISCONNECTED);
        // pause a second and then attempt to reconnect
        delay(1000);
        WiFi.begin(ssid, password);
        break;

      default:
        break;
    }
  });

  WiFi.begin(ssid, password);
}

void updateClock() {
  static uint32_t lastTick;
  if (millis() - lastTick >= 1000) {
    lastTick = millis();
    now = rtc.now();
  }
}

void updateDisplay() {
  static wl_status_t lastStatus = WL_DISCONNECTED;
  static uint32_t lastClock;
  static uint32_t lastRssi;

  if (WiFi.status() != lastStatus) {
    lastStatus = WiFi.status();
    println("WiFi status: %s", wifiStatusString(lastStatus));
    lcd.print(0, wifiStatusString(lastStatus));
    if (lastStatus == WL_CONNECTED) {
      lcd.print(1, WiFi.localIP().toString());
    }
    else {
      lcd.clear(1); // clear ip
    }
  }

  if (lastStatus == WL_CONNECTED && millis() - lastRssi >= 3000) {
    lastRssi = millis();
    lcd.printf(0, 13, "%ld dBm", WiFi.RSSI());
  }

  if (millis() - lastClock >= 1000) {
    lastClock = millis();
    lcd.printf(3, "%s %02d:%02d:%02d",
               DOW[now.dayOfTheWeek()],
               now.hour(), now.minute(), now.second());

    if (!logger.empty()) {
      Record r = logger.last();
      DateTime dt(r.timestamp);
      lcd.printf(2, "%s @ %02d:%02d",
                 eventToString(r.event),
                 dt.hour(), dt.minute());
    }
  }
}

void setupServer() {
  String index_html = R"(
    <html>
      <head><meta http-equiv="refresh" content="5"></head>
      <body>
        <h2>
          Garage is %door%
          <p>
          Toggle <a href="/activate">Garage Opener</a>
          <p>
          Toggle <a href="/backlight">Backlight</a>
          <p>
          %ssid% (%rssi% dBm)
          <p>
          %now%
        </h2>
        <pre>
%eventlog_10%
        </pre>
        <!--<a href="/logs">More Logs</a>-->
      </body>
    </html>
  )";

  String logs_html = R"(
    <html>
      <body>
        <h2>
          <a href="/">Home</a>
          <p>
          Garage is %door%
          <p>
          %now%
          <p>
        </h2>
        <pre>
%eventlog%
        </pre>
      </body>
    </html>
  )";

  server.on("/", HTTP_GET, [index_html](AsyncWebServerRequest *request) {
    led.blink(1);
    request->send(200, "text/html", injectVars(index_html));
  });

  server.on("/logs", HTTP_GET, [logs_html](AsyncWebServerRequest *request) {
    led.blink(1);
    request->send(200, "text/html", injectVars(logs_html));
  });

  server.on("/backlight", HTTP_GET, [](AsyncWebServerRequest *request) {
    led.blink(1);
    lcd.toggleBacklight();
    AsyncWebServerResponse *response = request->beginResponse(302);
    response->addHeader("Location", "/");
    request->send(response);
  });

  server.on("/activate", HTTP_GET, [](AsyncWebServerRequest *request) {
    led.blink(1);
    activateOpener();
    AsyncWebServerResponse *response = request->beginResponse(302);
    response->addHeader("Location", "/");
    request->send(response);
  });

  server.on("/api/status", HTTP_GET, [index_html](AsyncWebServerRequest *request) {
    led.blink(1);
    String json = R"({"door": "%door%", "timestamp": %timestamp%, "rssi": %rssi%})";
    request->send(200, "application/json", injectVars(json));
  });

  server.on("/api/backlight", HTTP_GET, [](AsyncWebServerRequest *request) {
    led.blink(1);
    lcd.toggleBacklight();
    request->send(200, "application/json", R"({"status": "ok"})");
  });

  server.on("/api/activate", HTTP_GET, [](AsyncWebServerRequest *request) {
    led.blink(1);
    activateOpener();
    request->send(200, "application/json", R"({"status": "ok"})");
  });

  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
  });

  server.begin();
}

String injectVars(const String& str) {
  String ret = String(str);
  int start = -1;
  for (int i = 0; i < str.length(); i++) {
    if (str[i] == '%') {
      if (start == -1) { // found start of token
        start = i;
      }
      else { // found end of token
        String token = str.substring(start, i + 1);
        if (token.length() > 2) {
          String var = token.substring(1, token.length() - 1);
          ret.replace(token, getVar(var));
        }
        start = -1;
      }
    }
  }
  return ret;
}

String getVar(const String& var) {
  if (var == "door") {
    return digitalRead(REED_SWITCH_PIN) == LOW ? "closed" : "open";
  }
  else if (var == "ssid") {
    return WiFi.SSID();
  }
  else if (var == "rssi") {
    return String(WiFi.RSSI());
  }
  else if (var == "timestamp") {
    return String(now.unixtime());
  }
  else if (var == "now") {
    return now.timestamp();
  }
  else if (var == "eventlog_10") {
    // TODO loading logs from FRAM (i2c) in ISR causes crashes
    // return getLogs(10, true);
    return recentLogs ? recentLogs : "";
  }
  else if (var == "eventlog") {
    // note: FRAM takes about 3ms to read each record
    // return getLogs(INT_MAX, false);
    return "";
  }
  return "";
}

String getLogs(int maxCount, bool reverse) {
  int i = 0;
  String s = "";
  logger.doEach([&](uint32_t timestamp, uint8_t event) {
    s.concat(DateTime(timestamp).timestamp());
    s.concat(" :: ");
    s.concat(eventToString(event));
    s.concat("\n");
  }, maxCount, reverse);
  return s;
}

static const char* eventToString(uint8_t event) {
  switch ((EventType)event) {
    case BOOTED: return "Booted";
    case CONNECTED: return "Connected";
    case DISCONNECTED: return "Disconnected";
    case DOOR_OPENED: return "Opened";
    case DOOR_CLOSED: return "Closed";
    default: return "Unknown";
  }
}

void sendPush(String title, String message) {
  println("Send push: %s", message);
#ifndef DEV_MODE
  struct PushSaferInput input;
  input.device = "a"; // all devices
  input.title = title;
  input.message = message;
  input.url = "http://garage.local";
  pusher.sendEvent(input);
#endif
}

void activateOpener() {
  println("Toggle garage opener");
#ifndef DEV_MODE
    digitalWrite(OPENER_PIN, HIGH);
    delay(50);
    digitalWrite(OPENER_PIN, LOW);
#endif
}

void checkReedSwitch() {
  reedSwitch.update();
  if (reedSwitch.fell()) {
      log(EventType::DOOR_CLOSED);
      sendPush("Garage", "Door has closed");
      lcd.backlight(true);
      led.blink(2);
  }
  else if (reedSwitch.rose()) {
      log(EventType::DOOR_OPENED);
      sendPush("Garage", "Door has opened");
      lcd.backlight(true);
      led.blink(2);
  }
}

void checkTouchSwitch() {
  static uint32_t lastToggle;
  static uint32_t lastTouch;
  static uint32_t lastCheck;
  static int count;

  // sample capacitive switch every 50 ms
  if (millis() - lastCheck >= 50) {
    lastCheck = millis();
    count = touchRead(TOUCH_PIN) < TOUCH_THRESHOLD ? count + 1 : 0;
    // if still touched after 3 cycles, activate
    if (count > 2) {
      // and last toggle was more than 3s ago
      if (millis() - lastToggle >= 1000) {
        lastToggle = millis();
        lcd.toggleBacklight();
        count = 0;
      }
    }
  }

  // debug trace to help calibrate threshold
  // static uint32_t lastDebug;
  // if (millis() - lastDebug >= 500) {
  //   lastDebug = millis();
  //   println("Touch sensor: %d", touchRead(TOUCH_PIN));
  // }
}

void log(EventType event) {
#ifdef DEV_MODE
  if (event == EventType::BOOTED) {
    println("Booted in DEV_MODE");
    recentLogs = getLogs(10, false);
  }
#else
  logger.write(now.unixtime(), event);
  recentLogs = getLogs(10, false);
#endif
}
