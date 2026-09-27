#include "common/abstractions/asset_manager.h"

#include <atomic>
#include <thread>

namespace common {

void waitForAssetToLoad(const std::atomic<AssetManager::LoadState>& loadState) {
  while (loadState.load(std::memory_order_acquire) == AssetManager::LoadState::PENDING) {
    std::this_thread::yield();
  }
}

void waitForAssetUntilChangesState(const std::atomic<AssetManager::LoadState>& loadState,
                                   AssetManager::LoadState unexpectedState) {
  while (loadState.load(std::memory_order_acquire) == unexpectedState) {
    std::this_thread::yield();
  }
}

void waitForAssetUntilReachesState(
    const std::atomic<AssetManager::LoadState>& loadState, AssetManager::LoadState expectedState) {
  while (loadState.load(std::memory_order_acquire) < expectedState) {
    std::this_thread::yield();
  }
}

}  // namespace common
