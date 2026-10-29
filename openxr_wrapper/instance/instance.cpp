#include "instance.h"

#include <algorithm>
#include <unordered_set>

#include "lib/buffer/buffer.h"
#include "openxr_wrapper/platform/platform.h"
#include "openxr_wrapper/util/check.h"

namespace xrw {

Instance::Instance(XrInstance instance, std::unordered_set<std::string>&& extensions)
  : _instance(instance), _extensions(std::move(extensions)) {}

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

std::unordered_set<std::string> getSupportedOpenXRExtensions() {
  uint32_t extensionCount;
  CHECK_XRCMD(xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr),
              "Failed to xrEnumerateInstanceExtensionProperties.");

  lib::Buffer<XrExtensionProperties> extensions(
      extensionCount, XrExtensionProperties{XR_TYPE_EXTENSION_PROPERTIES});

  CHECK_XRCMD(xrEnumerateInstanceExtensionProperties(
                  nullptr, extensionCount, &extensionCount, extensions.data()),
              "Failed to xrEnumerateInstanceExtensionProperties.");

  std::unordered_set<std::string> extensionSet;
  extensionSet.reserve(extensionCount);

  for (const auto& ext : extensions) {
    extensionSet.emplace(ext.extensionName);
  }

  return extensionSet;
}

}  // namespace

std::unique_ptr<Instance> Instance::create(
    std::string_view engineName, const Platform& platform, const GraphicsPlugin& graphicsPlugin) {
  assert(engineName.size() < XR_MAX_APPLICATION_NAME_SIZE);
  lib::Buffer<const char*> mergedExtensions = mergeExtensions(
      platform.getInstanceExtensions(), graphicsPlugin.getOpenXrInstanceExtensions());
  std::vector<const char*> openxrExtensions;
  std::unordered_set<std::string> supportedExtensions = getSupportedOpenXRExtensions();
  if (supportedExtensions.contains(XR_EXT_EYE_GAZE_INTERACTION_EXTENSION_NAME)) {
    openxrExtensions.push_back(XR_EXT_EYE_GAZE_INTERACTION_EXTENSION_NAME);
  }
  lib::Buffer<const char*> allExtensions = mergeExtensions(mergedExtensions, openxrExtensions);
  XrInstanceCreateInfo create_info = {
    .type = XR_TYPE_INSTANCE_CREATE_INFO,
    .next = platform.getInstanceCreateExtension(),
    .applicationInfo = {.apiVersion = XR_CURRENT_API_VERSION},
    .enabledExtensionCount = static_cast<uint32_t>(allExtensions.size()),
    .enabledExtensionNames = allExtensions.data()};
  std::memcpy(create_info.applicationInfo.applicationName, engineName.data(), engineName.size());

  XrInstance instance;
  CHECK_XRCMD(xrCreateInstance(&create_info, &instance), "Failed to create XrInstance.");
  return std::unique_ptr<Instance>(new Instance(
      instance,
      std::unordered_set<std::string>(openxrExtensions.cbegin(), openxrExtensions.cend())));
}

XrInstance Instance::getXrInstance() const {
  return _instance;
}

bool Instance::isExtensionEnabled(std::string_view extensionName) const noexcept {
  return _extensions.contains(std::string{extensionName});
}

}  // namespace xrw
