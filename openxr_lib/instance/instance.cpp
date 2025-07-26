#include "instance.h"
#include "openxr_lib/platform/platform.h"

namespace oxr {

Instance::Instance(XrInstance instance) : _instance(instance)  {}

Instance::~Instance() {
  if (_instance != XR_NULL_HANDLE) {
    xrDestroyInstance(_instance);
  }
}

std::unique_ptr<Instance> Instance::create(std::string_view engineName, const Platform* platform, std::span<const char* const> requiredExtensions) {
  XrInstanceCreateInfo create_info = {
      .type = XR_TYPE_INSTANCE_CREATE_INFO,
      .next = platform->getInstanceCreateExtension(),
      .enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
      .enabledExtensionNames = requiredExtensions.data(),
      .applicationInfo = {
        .apiVersion = XR_CURRENT_API_VERSION
      }
  };
  strcpy(create_info.applicationInfo.applicationName, engineName.data());

  XrInstance instance;
  if (XrResult result = xrCreateInstance(&create_info, &instance); result != XR_SUCCESS) {
    // Return error
  }
  return std::unique_ptr<Instance>(new Instance(instance));
}

} // oxr