#include <SPI.h>
#include <WiFiNINA.h>
#include <Wire.h>
#include <Print.h>
#include <Bounce2.h>
#include <Pushsafer.h>

#include "Lcd.h"
#include "Led.h"

#include "Secrets.h"
char ssid[] = SECRET_SSID;
char password[] = SECRET_PASS;
char apiKey[] = SECRET_API_KEY;

#define LED_PIN 13
#define BUTTON_PIN 2

int wifiStatus = WL_IDLE_STATUS;
WiFiServer server(80);

WiFiClient pushClient;
Pushsafer pusher(apiKey, pushClient);

Led led(LED_PIN);
Bounce button;
Lcd lcd;

char buf[20] = {0};

void setup() {
  Serial.begin(9600);

  led.init();
  led.blink(5);

  button.attach(BUTTON_PIN, INPUT_PULLDOWN);
  button.interval(25);

  lcd.init();
  // lcd.sleepAfter(10000);
  lcd.print(0, "Hello world!");
  lcd.backlight(true);

  initWifi();
}

void loop() {
  led.update();
  lcd.update();

  button.update();
  if (button.fell()) {
      led.blink(1);
      // lcd.toggleBacklight();
      WiFi.end();
  }

  updateWifi();

  if (wifiStatus == WL_CONNECTED) {
    WiFiClient client = server.available();
    if (client) {
      handleWebRequest(client);
    }
  }
}

void initWifi() {
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  Serial.print("Attempting to connect to Network named: ");
  Serial.println(ssid);
  lcd.print(0, ssid);
  // lcd.print(1, "Connecting...");

  WiFi.setTimeout(0); // do not block!
  wifiStatus = WiFi.begin(ssid, password);
  printWifiStatus(wifiStatus);
}

void updateWifi() {
  int status = WiFi.status();
  if (status != wifiStatus) {
    wifiStatus = status;
    printWifiStatus(wifiStatus);
    lcd.print(1, statusString(wifiStatus));

    if (status == WL_CONNECTED) {
      server.begin();
      // print ip address once, upon connection
      WiFi.localIP().printTo(*(lcd.getPrinter(0, 2)));
      led.blink(2);
    }
    else if (status == WL_CONNECT_FAILED
             || status == WL_CONNECTION_LOST
             || status == WL_DISCONNECTED
             || status == WL_IDLE_STATUS) {
      lcd.clear(2); // clear ip
      lcd.clear(3); // clear rssi
      wifiStatus = WiFi.begin(ssid, password);
    }
  }

  if (wifiStatus == WL_CONNECTED) {
    // print signal strength every second or so
    if (millis() % 1000 < 5) {
      sprintf(buf, "%ld dBm", WiFi.RSSI());
      lcd.print(3, buf);
    }
  }
}

void printWifiStatus(uint8_t status) {
  Serial.print("Wifi status: ");
  Serial.print(statusString(status));
  Serial.print(" (");
  Serial.print(status);
  Serial.println(")");
}

const char *statusString(uint8_t status) {
  switch (status) {
    case WL_NO_MODULE: return "No Module";
    case WL_IDLE_STATUS: return "Idle";
    case WL_NO_SSID_AVAIL: return "Searching...";
    case WL_SCAN_COMPLETED: return "Scan Completed";
    case WL_CONNECTED: return "Connected";
    case WL_CONNECT_FAILED: return "Connect Failed";
    case WL_CONNECTION_LOST: return "Connection Lost";
    case WL_DISCONNECTED: return "Disconnected";
    case WL_AP_LISTENING: return "AP Listening";
    case WL_AP_CONNECTED: return "AP Connected";
    case WL_AP_FAILED: return "AP Failed";
    default: return "Unknown Error";
  }
}

void handleWebRequest(WiFiClient client) {
    Serial.println("new client");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        //Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            sendResponse(client);
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            if (currentLine.startsWith("GET")) {
              handleCommand(currentLine.substring(5, currentLine.indexOf(' ', 5)));
            }
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("client disonnected");
}

void sendResponse(WiFiClient client) {
  // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
  // and a content-type so the client knows what's coming, then a blank line:
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println();

  // the content of the HTTP response follows the header:
  client.print("Turn <a href=\"/H\">LED on</a><br>");
  client.print("Turn <a href=\"/L\">LED off</a><br>");
  client.print("<p>");
  client.print("Toggle <a href=\"/B\">backlight</a><br>");
  client.print("<p>");
  client.print("Send <a href=\"/MSG?Hello\">a message</a><br>");

  // The HTTP response ends with another blank line:
  client.println();
}

void handleCommand(String cmd) {
  Serial.print("Got command: ");
  Serial.println(cmd);

  if (cmd.equalsIgnoreCase("H")) {
    led.blink(1);
    led.on();
  }
  else if (cmd.equalsIgnoreCase("L")) {
    led.blink(1);
    led.off();
  }
  else if (cmd.equalsIgnoreCase("B")) {
    led.blink(1);
    lcd.toggleBacklight();
  }
  else if (cmd.startsWith("MSG?")) {
    cmd.replace("%20", " ");
    cmd.substring(4).toCharArray(buf, 20);
    lcd.print(3, buf);
    lcd.backlight(true);
    led.blink(1);
    // sendPush("Garage Door", buf);
  }
}

void sendPush(String title, String message) {
  struct PushSaferInput input;
  input.title = title;
  input.message = message;
  // input.sound = "8";
  // input.vibration = "1";
  // input.icon = "1";
  // input.iconcolor = "#FFCCCC";
  // input.priority = "1";
  input.device = "a"; // all devices
  // input.url = "https://www.pushsafer.com";
  // input.urlTitle = "Open Pushsafer.com";
  // input.picture = "";
  // input.picture2 = "";
  // input.picture3 = "";
  // input.time2live = "";
  // input.retry = "";
  // input.expire = "";
  // input.answer = "";
  pusher.sendEvent(input);
}
