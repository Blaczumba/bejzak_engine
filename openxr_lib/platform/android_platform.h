#pragma once

#include "platform.h"

#include <openxr/openxr.h>
#define XR_USE_PLATFORM_ANDROID
#include <jni.h>
#include <openxr/openxr_platform.h>

#include <span>
#include <string_view>

namespace oxr {

struct AndroidData {
  void *application_vm;
  void *application_activity;
};

class AndroidPlatform : public Platform {
 public:
  explicit AndroidPlatform(const AndroidData& data);

  std::span<const char* const> getInstanceExtensions() const override;

  const XrBaseInStructure* getInstanceCreateExtension() const override;

 private:
  XrInstanceCreateInfoAndroidKHR _instance_create_info_android;
};

} // oxr
