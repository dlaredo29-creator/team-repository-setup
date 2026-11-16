#include <cassert>
#include <limits.h>
#include "distance_state.h"

int main() {
  DistanceStateTracker tracker;
  assert(tracker.update(0, 100.0f) == DISTANCE_DETECTING);
  assert(tracker.update(1999, 100.0f) == DISTANCE_DETECTING);
  assert(tracker.update(2000, 100.0f) == DISTANCE_RUSHING);
  assert(tracker.update(5999, 100.0f) == DISTANCE_RUSHING);
  assert(tracker.update(6000, 100.0f) == DISTANCE_UNHURRIED);

  assert(tracker.update(6001, 130.0f) == DISTANCE_UNHURRIED);
  assert(tracker.update(6002, 130.1f) == DISTANCE_UNHURRIED);
  assert(tracker.update(7000, 100.0f) == DISTANCE_UNHURRIED);
  assert(tracker.update(7001, 130.1f) == DISTANCE_UNHURRIED);
  assert(tracker.update(12000, 400.0f) == DISTANCE_UNHURRIED);
  assert(tracker.update(12001, 400.0f) == DISTANCE_NOBODY);

  assert(tracker.update(11003, 100.0f) == DISTANCE_DETECTING);
  assert(tracker.update(13003, 100.0f) == DISTANCE_RUSHING);
  assert(tracker.update(15003, 400.0f) == DISTANCE_RUSHING);
  assert(tracker.update(20003, 400.0f) == DISTANCE_NOBODY);

  DistanceStateTracker wraparoundTracker;
  const unsigned long nearWrap = ULONG_MAX - 1000UL;
  assert(wraparoundTracker.update(nearWrap, 100.0f) == DISTANCE_DETECTING);
  assert(wraparoundTracker.update(nearWrap + 2000UL, 100.0f) ==
         DISTANCE_RUSHING);
  assert(wraparoundTracker.update(nearWrap + 6000UL, 100.0f) ==
         DISTANCE_UNHURRIED);
}