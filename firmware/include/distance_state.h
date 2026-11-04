#ifndef DISTANCE_STATE_H
#define DISTANCE_STATE_H

enum DistanceState {
  DISTANCE_CLOSE,
  DISTANCE_DETECTED,
  DISTANCE_NOBODY
};

constexpr float CLOSE_DISTANCE_CM = 7.5f;
constexpr float DETECTED_DISTANCE_CM = 30.0f;

inline float getUpProgress(float distanceCm) {
  if (distanceCm <= CLOSE_DISTANCE_CM) return 1.0f;
  if (distanceCm <= DETECTED_DISTANCE_CM) return 0.5f;
  return 0.0f;
}

inline DistanceState classifyDistance(float distanceCm) {
  if (distanceCm <= CLOSE_DISTANCE_CM) {
    return DISTANCE_CLOSE;
  }
  if (distanceCm <= DETECTED_DISTANCE_CM) {
    return DISTANCE_DETECTED;
  }
  return DISTANCE_NOBODY;
}

#endif