#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include "pir_state.h"

U8G2_SSD1306_128X64_NONAME_1_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

const uint8_t PIR_PIN = 10;
const unsigned long FRAME_INTERVAL_MS = 33;

PirStateTracker stateTracker;
PirState currentState = PIR_NOBODY;
unsigned long lastFrameMs = 0;
unsigned long motionSinceMs = 0;
bool trackingMotion = false;

void showPirState(PirState state, bool motionDetected,
                  unsigned long motionHeldMs) {
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
  activityLabel = motionDetected ? "MOTION DETECTED" : "MOTION ENDED";

  oled.firstPage();
  do {
    oled.setFont(u8g2_font_6x12_tr);
    const int activityWidth = oled.getStrWidth(activityLabel);
    oled.drawStr((128 - activityWidth) / 2, 12, activityLabel);

    char reading[24];
    snprintf(reading, sizeof(reading), "PIR: %d  TIME: %lus",
             motionDetected ? 1 : 0, motionHeldMs / 1000UL);
    oled.drawStr((128 - oled.getStrWidth(reading)) / 2, 38, reading);

    const int paceWidth = oled.getStrWidth(paceLabel);
    oled.drawStr((128 - paceWidth) / 2, 63, paceLabel);
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
  const bool motionDetected = digitalRead(PIR_PIN) == HIGH;
  if (motionDetected && !trackingMotion) {
    trackingMotion = true;
    motionSinceMs = nowMs;
  } else if (!motionDetected) {
    if (trackingMotion) {
      motionSinceMs = nowMs - motionSinceMs;
    }
    trackingMotion = false;
  }

  const PirState nextState = stateTracker.update(nowMs, motionDetected);

  if (nextState != currentState) {
    currentState = nextState;
    oled.setPowerSave(currentState == PIR_NOBODY);
    printPirState(currentState);
  }

  if (currentState != PIR_NOBODY &&
      nowMs - lastFrameMs >= FRAME_INTERVAL_MS) {
    const unsigned long motionHeldMs =
        trackingMotion ? nowMs - motionSinceMs : motionSinceMs;
    showPirState(currentState, motionDetected, motionHeldMs);
    lastFrameMs = nowMs;
  }
}
