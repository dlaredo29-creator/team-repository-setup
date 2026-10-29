// oled_demo.ino: bench.ino plus a 0.96in OLED readout, for the demo.
//
// Same serial output as bench.ino ("cm 42.5" lines at 115200), so
// sense-serial.js works unchanged. The screen is extra. On the two-colour
// panels the top 16 rows are yellow: that strip is the status (state badge,
// readings per second). The blue below is the data: the distance, a sonar ping
// whose blip sits at the measured range, and a scrolling trace of the history.
//
// Wiring, Arduino Nano:
//   HC-SR04  VCC -> 5V    GND -> GND    TRIG -> D9    ECHO -> D10
//   OLED     VCC -> 5V    GND -> GND    SDA  -> A4    SCL  -> A5
// The OLED is an SSD1306, 128 x 64, I2C at 0x3C. Needs the U8g2 library.

#include <U8g2lib.h>
#include <Wire.h>
#include <math.h>

// Page mode: draws the screen in eight strips with a 128-byte buffer, because
// the Nano has 2 KB of RAM and a full frame would take half of it.
U8G2_SSD1306_128X64_NONAME_1_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

const int TRIG = 9;
const int ECHO = 10;

const float CLOSE_CM = 6.0;
const float DETECTED_CM = 30.0;
const float MAX_CM = 200.0;      // top of the graph and the sonar range
const uint8_t FIGURE_POINT_COUNT = 12;
const uint8_t FIGURE_POSE_COUNT = 5;
const float FIGURE_RISE_RATE_PER_SECOND = 0.55f;

const int GRAPH_H = 10;          // the trace, rows 53 to 63
uint8_t hist[128];               // one column per reading, 0..GRAPH_H
uint8_t head = 0;

float smooth = MAX_CM;
float sweep = 0;                 // sonar sweep angle, degrees
int sweepDir = 1;
uint8_t ping = 0;                // expanding ring radius
uint8_t rate = 0;                // readings in the last second, for the header
uint8_t count = 0;
unsigned long rateStart = 0;
float figureProgress = 0.0f;
unsigned long lastFrameMs = 0;

float readCm() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  unsigned long us = pulseIn(ECHO, HIGH, 30000);
  return us ? us / 58.0 : 400.0;
}

void splash() {
  for (uint8_t r = 2; r < 70; r += 6) {
    oled.firstPage();
    do {
      oled.drawCircle(64, 32, r);
      if (r > 8) oled.drawCircle(64, 32, r - 8);
      oled.setFont(u8g2_font_6x12_tr);
      oled.drawStr(28, 36, "SENSOR LAYER");
    } while (oled.nextPage());
  }
}

void drawHeader(float cm) {
  // Rows 0-15, the yellow strip: state badge left, readings per second right.
  const char *label = cm < CLOSE_CM ? "CLOSE" : cm <= DETECTED_CM ? "MEDIUM" : "EMPTY";
  oled.setFont(u8g2_font_6x12_tr);
  const bool medium = cm >= CLOSE_CM && cm <= DETECTED_CM;
  int w = oled.getStrWidth(label) + 8 + (medium ? 12 : 0);
  if (cm < CLOSE_CM) {             // close: solid badge
    oled.drawRBox(0, 1, w, 13, 3);
    oled.setDrawColor(0);
    oled.drawStr(4, 11, label);
    oled.setDrawColor(1);
  } else if (medium) {
    oled.drawRFrame(0, 1, w, 13, 3);
    oled.drawBox(4, 8, 2, 3);
    oled.drawBox(7, 5, 2, 6);
    oled.drawBox(10, 7, 2, 4);
    oled.drawStr(15, 11, label);
  } else {
    oled.drawRFrame(0, 1, w, 13, 3);
    oled.drawStr(4, 11, label);
  }
  char buf[8];
  oled.setFont(u8g2_font_5x8_tr);
  itoa(rate, buf, 10);
  strcat(buf, "/s");
  int rw = oled.getStrWidth(buf);
  oled.drawStr(127 - rw, 10, buf);
  // a pulse that blinks once per reading, beside the rate
  if (head & 1) oled.drawDisc(127 - rw - 6, 7, 2);
  else oled.drawCircle(127 - rw - 6, 7, 2);
}

void drawNumber(float cm) {
  char buf[8];
  oled.setFont(u8g2_font_logisoso22_tn);
  // The smoothed value creeps toward 400 (no echo) but never reaches it.
  if (cm >= 399.0) {
    strcpy(buf, "---");
  } else {
    dtostrf(cm, 0, cm < 100 ? 1 : 0, buf);
  }
  oled.drawStr(0, 45, buf);
  int x = oled.getStrWidth(buf) + 2;
  oled.setFont(u8g2_font_5x8_tr);
  oled.drawStr(x, 45, "cm");
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

float getUpProgress(float cm) {
  if (cm < CLOSE_CM) return 1.0f;
  if (cm <= DETECTED_CM) return 0.5f;
  return 0.0f;
}

void approachFigure(float targetProgress, unsigned long nowMs) {
  const float maximumStep = (nowMs - lastFrameMs) *
                            (FIGURE_RISE_RATE_PER_SECOND / 1000.0f);
  const float difference = targetProgress - figureProgress;
  if (difference > maximumStep) {
    figureProgress += maximumStep;
  } else if (difference < -maximumStep) {
    figureProgress -= maximumStep;
  } else {
    figureProgress = targetProgress;
  }
}

void drawStateAccents(float cm) {
  if (cm < CLOSE_CM) {
    oled.drawLine(78, 22, 82, 25);
    oled.drawLine(77, 35, 82, 35);
    oled.drawLine(78, 48, 82, 45);
    oled.drawLine(118, 25, 123, 22);
    oled.drawLine(118, 35, 124, 35);
    oled.drawLine(118, 45, 123, 48);
  } else if (cm <= DETECTED_CM) {
    const int bob = (millis() / 120) % 2;
    oled.drawCircle(80, 24 + bob, 2);
    oled.drawLine(119, 26 - bob, 124, 23 - bob);
    oled.drawLine(78, 46, 82, 46);
    oled.drawLine(119, 45, 124, 48);
  } else {
    oled.drawCircle(80, 45, 2);
    oled.drawCircle(120, 45, 2);
    oled.drawDisc(86, 50, 1);
    oled.drawDisc(115, 50, 1);
  }
}

void drawSonar(float cm) {
  const int ox = 99, oy = 50, R = 28;
  // range rings and the frame of the fan
  oled.drawCircle(ox, oy, R / 3, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT);
  oled.drawCircle(ox, oy, 2 * R / 3, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT);
  oled.drawCircle(ox, oy, R, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT);
  // the outgoing ping
  if (ping < R) oled.drawCircle(ox, oy, ping, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT);
  // the sweep line
  float a = (sweep - 90) * DEG_TO_RAD;
  oled.drawLine(ox, oy, ox + R * cos(a), oy + R * sin(a));
  // the blip, at the measured range along the sweep
  if (cm < MAX_CM) {
    float r = cm / MAX_CM * R;
    int bx = ox + r * cos(a), by = oy + r * sin(a);
    oled.drawDisc(bx, by, cm < CLOSE_CM ? 3 : 2);
  }
}

void drawGraph() {
  // A trace, oldest at the left: a line reads cleaner than solid bars, on the
  // panel and in a photo of it.
  const int y0 = 63;
  for (uint8_t x = 1; x < 128; x++) {
    uint8_t a = hist[(head + x - 1) % 128];
    uint8_t b = hist[(head + x) % 128];
    oled.drawLine(x - 1, y0 - a, x, y0 - b);
  }
}

void setup() {
  Serial.begin(115200);          // sense-serial.js opens the port at this speed
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  oled.begin();
  oled.setBusClock(400000);
  splash();
  lastFrameMs = millis();
}

void loop() {
  float cm = readCm();

  Serial.print("cm ");
  Serial.print(cm, 1);
  Serial.println();

  smooth += (min(cm, 400.0f) - smooth) * 0.12;

  count++;
  if (millis() - rateStart >= 1000) {
    rate = count;
    count = 0;
    rateStart = millis();
  }

  // closer reads taller in the graph
  float c = constrain(smooth, 0, MAX_CM);
  hist[head] = (uint8_t)((1.0 - c / MAX_CM) * GRAPH_H);
  head = (head + 1) % 128;

  const unsigned long frameTimeMs = millis();
  approachFigure(getUpProgress(smooth), frameTimeMs);
  lastFrameMs = frameTimeMs;

  sweep += 6 * sweepDir;
  if (sweep > 60 || sweep < -60) sweepDir = -sweepDir;
  ping = (ping + 3) % 36;

  oled.firstPage();
  do {
    drawHeader(cm);
    drawNumber(smooth);
    drawStateAccents(smooth);
    drawStickFigure(100, 35, figureProgress);
    drawGraph();
  } while (oled.nextPage());
}
