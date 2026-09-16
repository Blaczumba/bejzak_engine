#include "common/abstractions/asset_manager.h"

#include <atomic>
#include <thread>

namespace common {

void waitForAssetToLoad(const std::atomic<AssetManager::LoadState>& loadState) {
  while (loadState.load(std::memory_order_acquire) == AssetManager::LoadState::PENDING) {
    std::this_thread::yield();
  }
}

}  // namespace common
