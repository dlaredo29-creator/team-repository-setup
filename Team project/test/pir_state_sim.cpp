#include <cassert>
#include <limits.h>
#include "pir_state.h"

int main() {
  PirStateTracker tracker;

  assert(tracker.update(0, false) == PIR_NOBODY);
  assert(tracker.update(100, true) == PIR_DETECTING);
  assert(tracker.update(2099, true) == PIR_DETECTING);
  assert(tracker.update(2100, true) == PIR_RUSHING);
  assert(tracker.update(5099, true) == PIR_RUSHING);
  assert(tracker.update(5100, true) == PIR_UNHURRIED);

  assert(tracker.update(6101, false) == PIR_UNHURRIED);
  assert(tracker.update(11099, false) == PIR_UNHURRIED);
  assert(tracker.update(11101, false) == PIR_NOBODY);
  assert(tracker.update(11102, true) == PIR_DETECTING);
  assert(tracker.update(13101, true) == PIR_DETECTING);
  assert(tracker.update(13102, true) == PIR_RUSHING);
  assert(tracker.update(16101, true) == PIR_RUSHING);
  assert(tracker.update(16102, true) == PIR_UNHURRIED);

  PirStateTracker wraparoundTracker;
  const unsigned long nearWrap = ULONG_MAX - 1000UL;
  assert(wraparoundTracker.update(nearWrap, true) == PIR_DETECTING);
  assert(wraparoundTracker.update(nearWrap + 2000UL, true) == PIR_RUSHING);
  assert(wraparoundTracker.update(nearWrap + 5000UL, true) == PIR_UNHURRIED);
  assert(wraparoundTracker.update(nearWrap + 6000UL, false) == PIR_UNHURRIED);
  assert(wraparoundTracker.update(nearWrap + 11000UL, false) == PIR_NOBODY);

  PirStateTracker shortMotionTracker;
  assert(shortMotionTracker.update(100, true) == PIR_DETECTING);
  assert(shortMotionTracker.update(500, false) == PIR_NOBODY);

  PirStateTracker rushingHoldTracker;
  assert(rushingHoldTracker.update(0, true) == PIR_DETECTING);
  assert(rushingHoldTracker.update(2000, true) == PIR_RUSHING);
  assert(rushingHoldTracker.update(2100, false) == PIR_RUSHING);
  assert(rushingHoldTracker.update(4099, false) == PIR_RUSHING);
  assert(rushingHoldTracker.update(4100, false) == PIR_NOBODY);
}
