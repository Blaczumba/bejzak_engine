#pragma once

#include <functional>
#include <memory>
#include <version>
#include <vulkan/vulkan.h>

#include "lib/thread/worker.h"
#include "vulkan/wrapper/memory_allocator/allocation.h"

struct DestroyerContext {
  VkDevice device = VK_NULL_HANDLE;
  VkAllocationCallbacks* allocationCallbacks = nullptr;
  MemoryAllocator* memoryAllocator = nullptr;
};

class ResourceDestroyer {
public:
// TODO: Change after std::move_only_function becomes a standard.
#ifdef __cpp_lib_move_only_function
  using Job = std::move_only_function<void(DestroyerContext)>;
#else
  using Job = std::function<void(DestroyerContext)>;
#endif  // __cpp_lib_move_only_function

  virtual ~ResourceDestroyer() = default;

  virtual void destroyResource(Job destroyResource) = 0;

  virtual void setupContext(VkDevice device, VkAllocationCallbacks* allocationCallbacks,
                            MemoryAllocator* memoryAllocator) = 0;
};

using ResourceDestroyerPtr = std::unique_ptr<ResourceDestroyer>;

class ThreadedResourceDestroyer : public ResourceDestroyer {
public:
  ThreadedResourceDestroyer() noexcept = default;

  ~ThreadedResourceDestroyer() override = default;

  void destroyResource(Job destroyResource) override;

  void setupContext(VkDevice device, VkAllocationCallbacks* allocationCallbacks,
                    MemoryAllocator* memoryAllocator) override;

private:
  lib::thread::Worker<8, DestroyerContext> _worker;
};

class ImmediateResourceDestroyer : public ResourceDestroyer {
public:
  ImmediateResourceDestroyer() noexcept = default;

  ~ImmediateResourceDestroyer() override = default;

  void destroyResource(Job destroyResource) override;

  void setupContext(VkDevice device, VkAllocationCallbacks* allocationCallbacks,
                    MemoryAllocator* memoryAllocator) override;

private:
  DestroyerContext _context{};
};
