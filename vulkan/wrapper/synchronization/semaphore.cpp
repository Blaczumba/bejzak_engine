#include "vulkan/wrapper/synchronization/semaphore.h"

#include <vulkan/vulkan.h>

#include "vulkan/wrapper/util/check.h"

namespace {

template <typename T>
void chainExtendedField(void** next, T& feature) {
  feature.pNext = *next;
  *next = (void*)&feature;
}

}  // namespace

Semaphore::Semaphore(const LogicalDevice& logicalDevice, VkSemaphore semaphore)
  : _logicalDevice(&logicalDevice), _semaphore(semaphore) {}

Semaphore Semaphore::create(
    const LogicalDevice& logicalDevice, const VkSemaphoreCreateInfo& createInfo) {
  VkSemaphore semaphore;
  CHECK_VKCMD(vkCreateSemaphore(logicalDevice.getVkDevice(), &createInfo, nullptr, &semaphore),
              "Failed to create VkSemaphore.");
  return Semaphore(logicalDevice, semaphore);
}

Semaphore::Semaphore(Semaphore&& other) noexcept
  : _semaphore(std::exchange(other._semaphore, VK_NULL_HANDLE)),
    _logicalDevice(std::exchange(other._logicalDevice, nullptr)) {}

Semaphore& Semaphore::operator=(Semaphore&& other) noexcept {
  if (this == &other) {
    return *this;
  }

  if (_semaphore != VK_NULL_HANDLE) {
    destroy();
  }

  _semaphore = std::exchange(other._semaphore, VK_NULL_HANDLE);
  _logicalDevice = std::exchange(other._logicalDevice, nullptr);
  return *this;
}

Semaphore::~Semaphore() {
  if (_semaphore != VK_NULL_HANDLE) {
    destroy();
  }
}

VkSemaphore Semaphore::getVkSemaphore() const noexcept {
  return _semaphore;
}

void Semaphore::destroy() {
  _logicalDevice->destroyResource([semaphore = _semaphore](DestroyerContext context) {
    vkDestroySemaphore(context.device, semaphore, context.allocationCallbacks);
  });
}

SemaphoreBuilder& SemaphoreBuilder::withType(VkSemaphoreType type, uint64_t initialValue) noexcept {
  _typeInfo = VkSemaphoreTypeCreateInfo{
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
    .semaphoreType = type,
    .initialValue = (type == VK_SEMAPHORE_TYPE_BINARY) ? 0 : initialValue};
  chainExtendedField(&_pNext, *_typeInfo);
}

Semaphore SemaphoreBuilder::build(
    const LogicalDevice& logicalDevice, VkSemaphoreCreateFlags flags) {
  _flags = flags;

  if (!_typeInfo.has_value()) {
    withType(VK_SEMAPHORE_TYPE_BINARY);
  }

  const VkSemaphoreCreateInfo createInfo{
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, .pNext = _pNext, .flags = _flags};
  return Semaphore::create(logicalDevice, createInfo);
}
