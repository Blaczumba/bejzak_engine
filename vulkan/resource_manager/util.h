#pragma once

#include <cstdint>
#include <vector>

template <typename StrongHandle>
StrongHandle getNextHandle(size_t elementsCount, std::vector<StrongHandle>& missingHandles) {
  if (missingHandles.empty()) {
    return StrongHandle(elementsCount);
  }

  StrongHandle it = missingHandles.back();
  missingHandles.pop_back();
  return it;
}

template <typename StrongHandle>
StrongHandle getNextHandleFromReclaimed(
    StrongHandle& nextHandle, std::vector<StrongHandle>& reclaimedHandles) {
  if (!reclaimedHandles.empty()) {
    StrongHandle handle = reclaimedHandles.back();
    reclaimedHandles.pop_back();
    return handle;
  }
  return nextHandle++;
}
