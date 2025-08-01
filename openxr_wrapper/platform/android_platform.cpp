#include "android_platform.h"

#define XR_USE_PLATFORM_ANDROID
#include <jni.h>
#include <openxr/openxr_platform.h>

#include <array>

namespace xrw {

AndroidPlatform::AndroidPlatform(const AndroidData &data) {
  PFN_xrInitializeLoaderKHR initialize_loader = nullptr;

  if (XR_SUCCEEDED(xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR",
                                         reinterpret_cast<PFN_xrVoidFunction*>(&initialize_loader)))) {
    XrLoaderInitInfoAndroidKHR loader_init_info_android;
    loader_init_info_android.type = XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR;
    loader_init_info_android.next = nullptr;
    loader_init_info_android.applicationVM = data.application_vm;
    loader_init_info_android.applicationContext = data.application_activity;
    initialize_loader(reinterpret_cast<const XrLoaderInitInfoBaseHeaderKHR*>(&loader_init_info_android));
  }

  _instance_create_info_android = {XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR};
  _instance_create_info_android.applicationVM = data.application_vm;
  _instance_create_info_android.applicationActivity = data.application_activity;
}

std::span<const char* const> AndroidPlatform::getInstanceExtensions() const {
  static constexpr std::array platformExtensions = { XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME };
  return platformExtensions;
}

const XrBaseInStructure* AndroidPlatform::getInstanceCreateExtension() const {
  return reinterpret_cast<const XrBaseInStructure *>(&_instance_create_info_android);
}

} // xrw
