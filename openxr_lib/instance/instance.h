#pragma once

#include <openxr/openxr.h>

#include "openxr_lib/platform/platform.h"

#include <memory>
#include <span>
#include <string_view>

namespace oxr {

class Instance {
  XrInstance _instance;

  Instance(XrInstance instance);

 public:
  ~Instance();

  static std::unique_ptr<Instance> create(std::string_view engineName, const Platform* platform, std::span<const char* const> requiredExtensions);
};

} // oxr

