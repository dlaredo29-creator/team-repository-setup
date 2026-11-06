#ifndef PIR_STATE_H
#define PIR_STATE_H

enum PirState {
  PIR_DETECTING,
  PIR_RUSHING,
  PIR_UNHURRIED,
  PIR_NOBODY
};

constexpr unsigned long PIR_RUSHING_HOLD_MS = 2000;
constexpr unsigned long PIR_UNHURRIED_HOLD_MS = 6000;

class PirStateTracker {
 public:
  PirState update(unsigned long nowMs, bool motionDetected) {
    if (!motionDetected) {
      trackingMotion_ = false;
      state_ = PIR_NOBODY;
      return state_;
    }

    if (!trackingMotion_) {
      trackingMotion_ = true;
      motionSinceMs_ = nowMs;
    }

    const unsigned long heldMs = nowMs - motionSinceMs_;
    if (heldMs >= PIR_UNHURRIED_HOLD_MS) {
      state_ = PIR_UNHURRIED;
    } else if (heldMs >= PIR_RUSHING_HOLD_MS) {
      state_ = PIR_RUSHING;
    } else {
      state_ = PIR_DETECTING;
    }

    return state_;
  }

 private:
  PirState state_ = PIR_NOBODY;
  unsigned long motionSinceMs_ = 0;
  bool trackingMotion_ = false;
};

#endif
