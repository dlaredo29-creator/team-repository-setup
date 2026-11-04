#include <Arduino.h>

const uint8_t PIR_PIN = 10;
const unsigned long REPORT_INTERVAL_MS = 100;
unsigned long lastReportMs = 0;

void setup() {
  Serial.begin(115200);
  pinMode(PIR_PIN, INPUT);
  Serial.println("D-PIR monitor on D10");
}

void loop() {
  const unsigned long nowMs = millis();
  if (nowMs - lastReportMs < REPORT_INTERVAL_MS) return;

  Serial.print(nowMs);
  Serial.print(" ms\tD10\t");
  Serial.println(digitalRead(PIR_PIN) == HIGH ? "MOTION (HIGH)" : "clear (LOW)");
  lastReportMs = nowMs;
}
