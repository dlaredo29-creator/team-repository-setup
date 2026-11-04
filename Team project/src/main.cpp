#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <math.h>
#include "distance_state.h"

U8G2_SSD1306_128X64_NONAME_1_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

const uint8_t TRIG_PIN = 9;
const uint8_t ECHO_PIN = 10;

const uint8_t FIGURE_POINT_COUNT = 12;
const uint8_t FIGURE_POSE_COUNT = 5;
const float FIGURE_RESPONSE_MS = 420.0f;
unsigned long lastSensorReadMs = 0;
unsigned long lastFrameMs = 0;

float figureProgress = 0.0f;
float latestDistanceCm = 400.0f;
DistanceStateTracker stateTracker;
DistanceState currentState = DISTANCE_NOBODY;

float readDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  const unsigned long echoTime = pulseIn(ECHO_PIN, HIGH, 30000);
  return echoTime == 0 ? 400.0f : echoTime / 58.0f;
}

static const int8_t FIGURE_POSES[FIGURE_POSE_COUNT][FIGURE_POINT_COUNT][2] = {
    {{0, 14}, {0, 9}, {0, 6}, {-9, 6}, {-14, 9}, {9, 6}, {14, 9},
     {0, -7}, {-6, -14}, {-9, -20}, {6, -14}, {9, -20}},
    {{-7, 9}, {-4, 6}, {-2, 4}, {-9, 2}, {-13, 6}, {5, 0}, {10, -1},
     {3, -1}, {8, 5}, {13, 10}, {10, -6}, {14, -11}},
    {{0, 1}, {0, 5}, {0, 7}, {-9, 6}, {-13, 10}, {9, 6}, {13, 10},
     {0, 12}, {-7, 16}, {-9, 19}, {7, 15}, {10, 19}},
    {{0, -11}, {0, -7}, {0, -5}, {-9, -3}, {-13, 2}, {9, -3}, {13, 2},
     {0, 5}, {-6, 11}, {-8, 18}, {6, 11}, {8, 18}},
    {{0, -15}, {0, -11}, {0, -9}, {-10, -5}, {-14, 0}, {10, -5}, {14, 0},
     {0, 3}, {-6, 10}, {-8, 18}, {6, 10}, {8, 18}}};

void drawStickFigure(int originX, int originY, float progress) {
  const float posePosition = constrain(progress, 0.0f, 1.0f) *
                             (FIGURE_POSE_COUNT - 1);
  uint8_t firstPose = (uint8_t)posePosition;
  float blend = posePosition - firstPose;
  if (firstPose >= FIGURE_POSE_COUNT - 1) {
    firstPose = FIGURE_POSE_COUNT - 2;
    blend = 1.0f;
  }
  blend = blend * blend * (3.0f - 2.0f * blend);

  int8_t points[FIGURE_POINT_COUNT][2];
  for (uint8_t point = 0; point < FIGURE_POINT_COUNT; ++point) {
    for (uint8_t axis = 0; axis < 2; ++axis) {
      const int8_t start = FIGURE_POSES[firstPose][point][axis];
      const int8_t end = FIGURE_POSES[firstPose + 1][point][axis];
      points[point][axis] = (int8_t)lroundf(start + (end - start) * blend);
    }
  }

  const int headX = originX + points[0][0];
  const int headY = originY + points[0][1];
  oled.drawCircle(headX, headY, 3);
  oled.drawLine(headX, headY + (points[1][1] > points[0][1] ? 3 : -3),
                originX + points[1][0], originY + points[1][1]);
  oled.drawLine(originX + points[1][0], originY + points[1][1],
                originX + points[2][0], originY + points[2][1]);
  oled.drawLine(originX + points[2][0], originY + points[2][1],
                originX + points[7][0], originY + points[7][1]);
  oled.drawLine(originX + points[2][0], originY + points[2][1],
                originX + points[3][0], originY + points[3][1]);
  oled.drawLine(originX + points[3][0], originY + points[3][1],
                originX + points[4][0], originY + points[4][1]);
  oled.drawLine(originX + points[2][0], originY + points[2][1],
                originX + points[5][0], originY + points[5][1]);
  oled.drawLine(originX + points[5][0], originY + points[5][1],
                originX + points[6][0], originY + points[6][1]);
  oled.drawLine(originX + points[7][0], originY + points[7][1],
                originX + points[8][0], originY + points[8][1]);
  oled.drawLine(originX + points[8][0], originY + points[8][1],
                originX + points[9][0], originY + points[9][1]);
  oled.drawLine(originX + points[7][0], originY + points[7][1],
                originX + points[10][0], originY + points[10][1]);
  oled.drawLine(originX + points[10][0], originY + points[10][1],
                originX + points[11][0], originY + points[11][1]);

  for (uint8_t point = 2; point < FIGURE_POINT_COUNT; ++point) {
    if (point == 4 || point == 6 || point == 9 || point == 11) continue;
    oled.drawDisc(originX + points[point][0], originY + points[point][1], 1);
  }
}

void approachFigure(float targetProgress, unsigned long nowMs) {
  const float elapsedMs = nowMs - lastFrameMs;
  const float smoothing = elapsedMs / (FIGURE_RESPONSE_MS + elapsedMs);
  figureProgress += (targetProgress - figureProgress) * smoothing;
}

void drawStateAccents(DistanceState state) {
  if (state == DISTANCE_RUSHING) {
    oled.drawFrame(52, 18, 24, 24);
    oled.drawVLine(64, 23, 11);
    oled.drawDisc(64, 38, 1);
  } else if (state == DISTANCE_UNHURRIED) {
    oled.drawCircle(43, 20, 2);
    oled.drawCircle(85, 20, 2);
    oled.drawLine(43, 46, 49, 46);
    oled.drawLine(79, 46, 85, 46);
  } else {
    oled.drawCircle(43, 20, 2);
    oled.drawCircle(85, 20, 2);
  }
}

void showSensorState(DistanceState state, float figurePoseProgress) {
  if (state == DISTANCE_NOBODY) {
    oled.setPowerSave(1);
    return;
  }
  oled.setPowerSave(0);

  const char *label;

  switch (state) {
    case DISTANCE_UNHURRIED:
      label = "UNHURRIED";
      break;
    case DISTANCE_RUSHING:
      label = "RUSHING";
      break;
    default:
      label = "DETECTING";
      break;
  }

  oled.firstPage();
  do {
    oled.setFont(u8g2_font_6x12_tr);
    const int labelWidth = oled.getStrWidth(label);
    oled.drawStr((128 - labelWidth) / 2, 12, label);
    drawStateAccents(state);
    if (state != DISTANCE_RUSHING) {
      drawStickFigure(64, 39, figurePoseProgress);
    }
  } while (oled.nextPage());
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  Wire.begin();
  Serial.println("I2C scan:");
  for (uint8_t address = 1; address < 127; ++address) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found 0x");
      Serial.println(address, HEX);
    }
  }
  oled.begin();
  lastSensorReadMs = millis();
  lastFrameMs = millis();
}

void loop() {
  const unsigned long nowMs = millis();
  if (nowMs - lastSensorReadMs >= 50) {
    latestDistanceCm = readDistanceCm();
    currentState = stateTracker.update(millis(), latestDistanceCm);
    lastSensorReadMs = millis();
  }

  const unsigned long frameTimeMs = millis();
  if (frameTimeMs - lastFrameMs >= 33) {
    const float targetProgress =
        currentState == DISTANCE_UNHURRIED
            ? 0.5f + 0.5f * sinf(frameTimeMs / 3000.0f)
            : 0.5f;
    approachFigure(targetProgress, frameTimeMs);
    showSensorState(currentState, figureProgress);
    lastFrameMs = frameTimeMs;
  }
  delay(1);
}