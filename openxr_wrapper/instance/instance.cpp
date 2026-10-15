#include "instance.h"

#include "lib/buffer/buffer.h"
#include "openxr_wrapper/platform/platform.h"
#include "openxr_wrapper/util/check.h"

namespace xrw {

Instance::Instance(XrInstance instance) : _instance(instance) {}

Instance::~Instance() {
  xrDestroyInstance(_instance);
}

namespace {

lib::Buffer<const char*> mergeExtensions(
    std::span<const char* const> extA, std::span<const char* const> extB) {
  lib::Buffer<const char*> mergedExtensions(extA.size() + extB.size());
  std::copy(std::cbegin(extA), std::cend(extA), mergedExtensions.begin());
  std::copy(std::cbegin(extB), std::cend(extB), mergedExtensions.begin() + extA.size());
  return mergedExtensions;
}

}  // namespace

std::unique_ptr<Instance> Instance::create(
    std::string_view engineName, const Platform& platform, const GraphicsPlugin& graphicsPlugin) {
  lib::Buffer<const char*> mergedExtensions = mergeExtensions(
      platform.getInstanceExtensions(), graphicsPlugin.getOpenXrInstanceExtensions());
  std::array openxrExtensions = {XR_EXT_EYE_GAZE_INTERACTION_EXTENSION_NAME};
  lib::Buffer<const char*> allExtensions = mergeExtensions(
      mergedExtensions, openxrExtensions);
  XrInstanceCreateInfo create_info = {
    .type = XR_TYPE_INSTANCE_CREATE_INFO,
    .next = platform.getInstanceCreateExtension(),
    .applicationInfo = {.apiVersion = XR_CURRENT_API_VERSION},
    .enabledExtensionCount = static_cast<uint32_t>(allExtensions.size()),
    .enabledExtensionNames = allExtensions.data()};
  std::strcpy(create_info.applicationInfo.applicationName, engineName.data());

  XrInstance instance;
  CHECK_XRCMD(xrCreateInstance(&create_info, &instance), "Failed to create XrInstance.");
  return std::unique_ptr<Instance>(new Instance(instance));
}

XrInstance Instance::getXrInstance() const {
  return _instance;
}

}  // namespace xrw
