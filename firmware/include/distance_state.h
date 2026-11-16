#ifndef DISTANCE_STATE_H
#define DISTANCE_STATE_H

enum DistanceState {
  DISTANCE_DETECTING,
  DISTANCE_RUSHING,
  DISTANCE_UNHURRIED,
  DISTANCE_NOBODY
};

constexpr float PERSON_MAX_DISTANCE_CM = 130.0f;
constexpr unsigned long RUSHING_HOLD_MS = 2000;
constexpr unsigned long UNHURRIED_HOLD_MS = 6000;
constexpr unsigned long NOBODY_TIMEOUT_MS = 5000;

class DistanceStateTracker {
 public:
  DistanceState update(unsigned long nowMs, float distanceCm) {
    // The HC-SR04 reports presence, not motion; readings in range stand in for
    // a continuously detected person.
    const bool personInRange =
        distanceCm > 0.0f && distanceCm <= PERSON_MAX_DISTANCE_CM;

    if (personInRange) {
      if (!trackingPerson_) {
        trackingPerson_ = true;
        personSinceMs_ = nowMs;
      }
      trackingFar_ = false;

      const unsigned long heldMs = nowMs - personSinceMs_;
      if (heldMs >= UNHURRIED_HOLD_MS) {
        state_ = DISTANCE_UNHURRIED;
      } else if (heldMs >= RUSHING_HOLD_MS) {
        state_ = DISTANCE_RUSHING;
      } else {
        state_ = DISTANCE_DETECTING;
      }
    } else {
      if (!trackingFar_) {
        trackingFar_ = true;
        farSinceMs_ = nowMs;
      }
      if (nowMs - farSinceMs_ >= NOBODY_TIMEOUT_MS) {
        state_ = DISTANCE_NOBODY;
        trackingPerson_ = false;
      }
    }

    return state_;
  }

 private:
  DistanceState state_ = DISTANCE_NOBODY;
  unsigned long personSinceMs_ = 0;
  unsigned long farSinceMs_ = 0;
  bool trackingPerson_ = false;
  bool trackingFar_ = false;
};

#endif