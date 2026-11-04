#include <cassert>
#include "distance_state.h"

int main() {
  assert(classifyDistance(0.0f) == DISTANCE_CLOSE);
  assert(classifyDistance(7.5f) == DISTANCE_CLOSE);
  assert(classifyDistance(7.6f) == DISTANCE_DETECTED);
  assert(classifyDistance(17.0f) == DISTANCE_DETECTED);
  assert(classifyDistance(30.0f) == DISTANCE_DETECTED);
  assert(classifyDistance(30.1f) == DISTANCE_NOBODY);
  assert(classifyDistance(400.0f) == DISTANCE_NOBODY);

  assert(getUpProgress(400.0f) == 0.0f);
  assert(getUpProgress(30.1f) == 0.0f);
  assert(getUpProgress(30.0f) == 0.5f);
  assert(getUpProgress(18.75f) == 0.5f);
  assert(getUpProgress(7.6f) == 0.5f);
  assert(getUpProgress(7.5f) == 1.0f);
  assert(getUpProgress(7.4f) == 1.0f);
  assert(getUpProgress(0.0f) == 1.0f);
}