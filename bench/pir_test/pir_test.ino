// PIR motion sensor test for an Arduino Nano.
// Connect sensor OUT to D10, VCC to 5V, and GND to GND.

const uint8_t PIR_PIN = 10;
const unsigned long WARMUP_MS = 60000UL;
const unsigned long CLEAR_STABLE_MS = 2000UL;

bool previousMotion = false;
bool waitingForClearMessageSent = false;
bool clearTiming = false;
bool armed = false;
unsigned long startupMs = 0;
unsigned long clearSinceMs = 0;

void setup() {
  Serial.begin(115200);
  pinMode(PIR_PIN, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  startupMs = millis();
  Serial.println("PIR warming up for 60 seconds; keep the area clear.");
}

void loop() {
  const unsigned long nowMs = millis();
  const bool motionDetected = digitalRead(PIR_PIN) == HIGH;
  digitalWrite(LED_BUILTIN, motionDetected ? HIGH : LOW);

  if (!armed) {
    if (nowMs - startupMs < WARMUP_MS) {
      delay(50);
      return;
    }

    if (motionDetected) {
      clearTiming = false;
      if (!waitingForClearMessageSent) {
        Serial.println("PIR output is HIGH; waiting for a stable clear signal.");
        waitingForClearMessageSent = true;
      }
    } else if (!clearTiming) {
      clearSinceMs = nowMs;
      clearTiming = true;
    } else if (nowMs - clearSinceMs >= CLEAR_STABLE_MS) {
      armed = true;
      previousMotion = false;
      Serial.println("PIR armed; waiting for motion.");
    }

    delay(50);
    return;
  }

  if (motionDetected != previousMotion) {
    Serial.println(motionDetected ? "MOTION DETECTED" : "No motion");
    previousMotion = motionDetected;
  }

  delay(50);
}
