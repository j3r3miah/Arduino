#include <stdarg.h>

char __utils_buf[128];

void print(char *fmt, ...) {
  va_list args;
  va_start (args, fmt );
  vsnprintf(__utils_buf, sizeof(__utils_buf), fmt, args);
  va_end (args);
  Serial.print(__utils_buf);
}

void println(char *fmt, ...) {
  va_list args;
  va_start (args, fmt );
  vsnprintf(__utils_buf, sizeof(__utils_buf), fmt, args);
  va_end (args);
  Serial.println(__utils_buf);
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
