#include <cassert>
#include <limits.h>
#include "pir_state.h"

int main() {
  PirStateTracker tracker;

  assert(tracker.update(0, false) == PIR_NOBODY);
  assert(tracker.update(100, true) == PIR_DETECTING);
  assert(tracker.update(2099, true) == PIR_DETECTING);
  assert(tracker.update(2100, true) == PIR_RUSHING);
  assert(tracker.update(6099, true) == PIR_RUSHING);
  assert(tracker.update(6100, true) == PIR_UNHURRIED);

  assert(tracker.update(6101, false) == PIR_NOBODY);
  assert(tracker.update(6102, true) == PIR_DETECTING);
  assert(tracker.update(8102, true) == PIR_RUSHING);
  assert(tracker.update(14102, true) == PIR_UNHURRIED);

  PirStateTracker wraparoundTracker;
  const unsigned long nearWrap = ULONG_MAX - 1000UL;
  assert(wraparoundTracker.update(nearWrap, true) == PIR_DETECTING);
  assert(wraparoundTracker.update(nearWrap + 2000UL, true) == PIR_RUSHING);
  assert(wraparoundTracker.update(nearWrap + 6000UL, true) == PIR_UNHURRIED);
}
