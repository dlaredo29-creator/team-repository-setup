#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <math.h>
#include "pir_state.h"

U8G2_SSD1306_128X64_NONAME_1_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

const uint8_t PIR_PIN = 10;
const unsigned long FRAME_INTERVAL_MS = 33;

PirStateTracker stateTracker;
PirState currentState = PIR_NOBODY;
unsigned long lastFrameMs = 0;

void showPirState(PirState state, unsigned long nowMs) {
  const char *activityLabel;
  const char *paceLabel;
  switch (state) {
    case PIR_RUSHING:
      paceLabel = "RUSHING";
      break;
    case PIR_UNHURRIED:
      paceLabel = "UNHURRIED";
      break;
    default:
      paceLabel = "DETECTING";
      break;
  }
  activityLabel = "MOTION DETECTED";

  oled.firstPage();
  do {
    oled.setFont(u8g2_font_6x12_tr);
    const int activityWidth = oled.getStrWidth(activityLabel);
    oled.drawStr((128 - activityWidth) / 2, 12, activityLabel);
    const int paceWidth = oled.getStrWidth(paceLabel);
    oled.drawStr((128 - paceWidth) / 2, 63, paceLabel);

    if (state == PIR_RUSHING) {
      oled.drawFrame(52, 20, 24, 25);
      oled.drawVLine(64, 25, 12);
      oled.drawDisc(64, 41, 1);
    } else {
      const int y = state == PIR_UNHURRIED
                        ? 34 + (int)(2.0f * sinf(nowMs / 1200.0f))
                        : 34;
      oled.drawCircle(64, y - 12, 4);
      oled.drawVLine(64, y - 7, 13);
      oled.drawLine(64, y - 2, 54, y + 4);
      oled.drawLine(64, y - 2, 74, y + 4);
      oled.drawLine(64, y + 6, 58, y + 17);
      oled.drawLine(64, y + 6, 70, y + 17);
    }
  } while (oled.nextPage());
}

void printPirState(PirState state) {
  switch (state) {
    case PIR_DETECTING:
      Serial.println("MOTION DETECTED");
      break;
    case PIR_RUSHING:
      Serial.println("RUSHING");
      break;
    case PIR_UNHURRIED:
      Serial.println("UNHURRIED");
      break;
    case PIR_NOBODY:
      Serial.println("NOBODY");
      break;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PIR_PIN, INPUT);
  Wire.begin();
  oled.begin();
  oled.setPowerSave(1);
  Serial.println("PIR sensor on D10");
}

void loop() {
  const unsigned long nowMs = millis();
  const PirState nextState =
      stateTracker.update(nowMs, digitalRead(PIR_PIN) == HIGH);

  if (nextState != currentState) {
    currentState = nextState;
    oled.setPowerSave(currentState == PIR_NOBODY);
    printPirState(currentState);
  }

  if (currentState != PIR_NOBODY &&
      nowMs - lastFrameMs >= FRAME_INTERVAL_MS) {
    showPirState(currentState, nowMs);
    lastFrameMs = nowMs;
  }
}
