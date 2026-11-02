#ifndef DISTANCE_STATE_H
#define DISTANCE_STATE_H

enum DistanceState {
  DISTANCE_CLOSE,
  DISTANCE_DETECTED,
  DISTANCE_NOBODY
};

constexpr float CLOSE_DISTANCE_CM = 6.0f;
constexpr float DETECTED_DISTANCE_CM = 30.0f;

inline float getUpProgress(float distanceCm) {
  const float progress =
      (DETECTED_DISTANCE_CM - distanceCm) /
      (DETECTED_DISTANCE_CM - CLOSE_DISTANCE_CM);
  if (progress < 0.0f) return 0.0f;
  if (progress > 1.0f) return 1.0f;
  return progress;
}

inline DistanceState classifyDistance(float distanceCm) {
  if (distanceCm < CLOSE_DISTANCE_CM) {
    return DISTANCE_CLOSE;
  }
  if (distanceCm <= DETECTED_DISTANCE_CM) {
    return DISTANCE_DETECTED;
  }
  return DISTANCE_NOBODY;
}

#endif