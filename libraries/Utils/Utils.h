#ifndef UTILS_H
#define UTILS_H

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

#endif
