// disable opener and push notifications during dev
#define NO_PUSH 1
#define NO_OPENER 1

#include <WiFi.h>
#include <RTClib.h>
#include <ESPmDNS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Pushsafer.h>
#include <Bounce2.h>

#include <WiFiUtils.h>
#include <Utils.h>

#include "Lcd.h"
#include "Led.h"
#include "EventLog.h"
#include "Secrets.h"

#define MDNS_NAME "garage"
#define LED_PIN LED_BUILTIN
#define REED_SWITCH_PIN 26
#define OPENER_PIN TODO

char DOW[7][12] = {
  "Sunday",
  "Monday",
  "Tuesday",
  "Wednesday",
  "Thursday",
  "Friday",
  "Saturday"
};

char ssid[] = WIFI_SSID;
char password[] = WIFI_PASS;
char pusherKey[] = PUSHER_API_KEY;

Lcd lcd;
RTC_DS3231 rtc;
Led led(LED_PIN);
Bounce reedSwitch;
AsyncWebServer server(80);
std::function<void()> serverAction;
WiFiClient pushClient;
Pushsafer pusher(pusherKey, pushClient);
EventLog logger;
DateTime now;

void setup() {
  Serial.begin(115200);
  setupClock();
  now = rtc.now();
  logger.write(now.unixtime(), EventType::BOOTED);

  led.init(5);
  lcd.init();
  lcd.sleepAfter(1000 * 60 * 3);
  lcd.backlight(true);

  reedSwitch.attach(REED_SWITCH_PIN, INPUT_PULLUP);
  reedSwitch.interval(25);

  setupServer();
}

void loop() {
  led.update();
  lcd.update();
  reedSwitch.update();
  updateClock();
  updateDisplay();

  if (reedSwitch.fell()) {
      logger.write(now.unixtime(), EventType::DOOR_CLOSED);
      sendPush("Garage", "Door has closed");
      lcd.backlight(true);
      led.blink(2);
  }
  else if (reedSwitch.rose()) {
      logger.write(now.unixtime(), EventType::DOOR_OPENED);
      sendPush("Garage", "Door has opened");
      lcd.backlight(true);
      led.blink(2);
  }

  if (serverAction != NULL) {
    serverAction();
    serverAction = NULL;
  }
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
    lcd.print(0, wifiStatusString(lastStatus));
    if (lastStatus == WL_CONNECTED) {
      logger.write(now.unixtime(), EventType::CONNECTED);
      println("Connected to %s: %s",
              WiFi.SSID().c_str(),
              WiFi.localIP().toString().c_str());
      lcd.print(1, WiFi.localIP().toString());
    }
    else {
      logger.write(now.unixtime(), EventType::DISCONNECTED);
      println("WiFi status: %s", wifiStatusString(lastStatus));
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
                 EventLog::toString(r.event),
                 dt.hour(), dt.minute());
    }
  }
}

void setupServer() {
  WiFi.begin(ssid, password);

  if (!MDNS.begin(MDNS_NAME)) {
    println("mDNS responder failed");
    while (true);
  }
  println("mDNS responder started");
  MDNS.addService("http", "tcp", 80);

  const char *index_html = R"(
      <head><meta http-equiv="refresh" content="5"></head>
      <h2>
      Garage is %door%<br>
      <p>
      Toggle <a href="/activate">Garage Opener</a><br>
      <p>
      Toggle <a href="/backlight">Backlight</a><br>
      <p>
      %ssid% (%rssi% dBm)<br>
      <p>
      %now%
      <h2>
      <pre>
%eventlog%
      </pre>
  )";

  server.on("/", HTTP_GET, [index_html](AsyncWebServerRequest *request) {
    led.blink(1);
    request->send_P(200, "text/html", index_html, templateVar);
  });

  server.on("/backlight", HTTP_GET, [](AsyncWebServerRequest *request) {
    led.blink(1);
    // since webserver is interrupt driven we can't talk to i2c here, so enqueue
    // the action to execute in a subsequent loop pass
    serverAction = [&]() { lcd.toggleBacklight(); };
    request->redirect("/");
  });

  server.on("/activate", HTTP_GET, [](AsyncWebServerRequest *request) {
    led.blink(1);
    activateOpener();
    request->redirect("/");
  });

  server.on("/api/status", HTTP_GET, [index_html](AsyncWebServerRequest *request) {
    led.blink(1);
    request->send_P(200, "application/json",
                    R"({"door": "%door%", "timestamp": %timestamp%, "rssi": %rssi%})",
                    templateVar);
  });

  server.on("/api/backlight", HTTP_GET, [](AsyncWebServerRequest *request) {
    led.blink(1);
    // since webserver is interrupt driven we can't talk to i2c here, so enqueue
    // the action to execute in a subsequent loop pass
    serverAction = [&]() { lcd.toggleBacklight(); };
    request->send(200, "application/json", R"({"status": "ok"})");
  });

  server.on("/api/activate", HTTP_GET, [](AsyncWebServerRequest *request) {
    led.blink(1);
    activateOpener();
    request->send(200, "application/json", R"({"status": "ok"})");
  });

  server.begin();
}

String templateVar(const String& var){
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
  else if (var == "eventlog") {
    String s = "";
    logger.doEach([&](uint32_t timestamp, EventType event) {
      s.concat(DateTime(timestamp).timestamp());
      s.concat(" :: ");
      s.concat(EventLog::toString(event));
      s.concat("\n");
    });
    return s;
  }
  return "";
}

void sendPush(String title, String message) {
#ifdef NO_PUSH
  Serial.print("Send push: ");
  Serial.println(message);
#else
  struct PushSaferInput input;
  input.device = "a"; // all devices
  input.title = title;
  input.message = message;
  input.url = "http://garage.local";
  pusher.sendEvent(input);
  // TODO check for success
#endif
}

void activateOpener() {
#ifdef NO_OPENER
  Serial.println("Toggle garage opener");
#else
    digitalWrite(OPENER_PIN, HIGH);
    delay(50);
    digitalWrite(OPENER_PIN, LOW);
#endif
}
