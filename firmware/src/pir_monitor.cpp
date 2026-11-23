#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <string.h>
#include "pir_state.h"

U8G2_SSD1306_128X64_NONAME_1_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

const uint8_t PIR_PIN = 10;
const unsigned long FRAME_INTERVAL_MS = 33;

PirStateTracker stateTracker;
PirState currentState = PIR_NOBODY;
unsigned long lastFrameMs = 0;
unsigned long motionSinceMs = 0;
bool trackingMotion = false;
bool testMode = false;
bool lastTestReading = false;
bool testReadingValid = false;
unsigned long testStartedMs = 0;
unsigned long lastTestLogMs = 0;
char commandBuffer[8];
uint8_t commandLength = 0;
bool commandOverflow = false;

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
    case PIR_NOBODY:
      paceLabel = "NOBODY";
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

void showTestReading(bool detected, unsigned long elapsedSeconds) {
  const char *reading = detected ? "1" : "0";
  char elapsed[16];
  snprintf(elapsed, sizeof(elapsed), "TEST %02lu:%02lu:%02lu",
           elapsedSeconds / 3600UL, (elapsedSeconds / 60UL) % 60UL,
           elapsedSeconds % 60UL);

  oled.firstPage();
  do {
    oled.setFont(u8g2_font_5x8_tr);
    oled.drawStr((128 - oled.getStrWidth(elapsed)) / 2, 9, elapsed);

    oled.setFont(u8g2_font_logisoso32_tn);
    const int width = oled.getStrWidth(reading);
    oled.drawStr((128 - width) / 2, 48, reading);

    const char *status = detected ? "MOTION" : "CLEAR";
    oled.setFont(u8g2_font_6x12_tr);
    oled.drawStr((128 - oled.getStrWidth(status)) / 2, 62, status);
  } while (oled.nextPage());
}

void printTestLog(unsigned long nowMs, bool detected) {
  const unsigned long elapsedSeconds = (nowMs - testStartedMs) / 1000UL;
  char entry[32];
  snprintf(entry, sizeof(entry), "%02lu:%02lu:%02lu,%d,%s",
           elapsedSeconds / 3600UL, (elapsedSeconds / 60UL) % 60UL,
           elapsedSeconds % 60UL, detected ? 1 : 0,
           detected ? "MOTION" : "CLEAR");
  Serial.println(entry);
}

void readSerialCommands() {
  while (Serial.available() > 0) {
    char input = (char)Serial.read();
    if (input == '\r') continue;

    if (input == '\n') {
      commandBuffer[commandLength] = '\0';
      if (!commandOverflow && strcmp(commandBuffer, "test") == 0) {
        testMode = true;
        testReadingValid = false;
        testStartedMs = millis();
        lastTestLogMs = testStartedMs;
        Serial.println("elapsed,raw,status");
      } else if (!commandOverflow &&
                 (strcmp(commandBuffer, "normal") == 0 ||
                  strcmp(commandBuffer, "un test") == 0)) {
        testMode = false;
        testReadingValid = false;
        stateTracker.reset();
        currentState = PIR_NOBODY;
        trackingMotion = false;
        motionSinceMs = 0;
      }
      commandLength = 0;
      commandOverflow = false;
      continue;
    }

    if (commandLength < sizeof(commandBuffer) - 1) {
      if (input >= 'A' && input <= 'Z') input += 'a' - 'A';
      commandBuffer[commandLength++] = input;
    } else {
      commandOverflow = true;
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PIR_PIN, INPUT);
  Wire.begin();

  uint8_t oledAddress = 0;
  for (uint8_t address = 1; address < 127; ++address) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0 &&
        (address == 0x3C || address == 0x3D)) {
      oledAddress = address;
      break;
    }
  }

  if (oledAddress != 0) {
    oled.setI2CAddress(oledAddress << 1);
    Serial.print("OLED initialized at 0x");
    Serial.println(oledAddress, HEX);
  } else {
    Serial.println("OLED not found at 0x3C or 0x3D; check VCC, GND, SDA, and SCL.");
  }

  oled.begin();
  oled.setPowerSave(0);
  showPirState(PIR_NOBODY, false, 0);
  Serial.println("PIR sensor on D10");
}

void loop() {
  readSerialCommands();
  const unsigned long nowMs = millis();
  const bool motionDetected = digitalRead(PIR_PIN) == HIGH;

  if (testMode) {
    oled.setPowerSave(0);
    const bool readingChanged =
        !testReadingValid || motionDetected != lastTestReading;
    const bool heartbeatDue = nowMs - lastTestLogMs >= 1000UL;
    if (readingChanged) {
      lastTestReading = motionDetected;
      testReadingValid = true;
    }
    if (readingChanged || heartbeatDue) {
      printTestLog(nowMs, motionDetected);
      showTestReading(motionDetected, (nowMs - testStartedMs) / 1000UL);
    }
    if (heartbeatDue) {
      lastTestLogMs = nowMs;
    }
    return;
  }

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
    printPirState(currentState);
  }

  oled.setPowerSave(currentState == PIR_NOBODY);

  if (currentState != PIR_NOBODY &&
      nowMs - lastFrameMs >= FRAME_INTERVAL_MS) {
    const unsigned long motionHeldMs =
        trackingMotion ? nowMs - motionSinceMs : motionSinceMs;
    showPirState(currentState, motionDetected, motionHeldMs);
    lastFrameMs = nowMs;
  }
}
