#pragma once

#include <memory>
#include <openxr/openxr.h>
#include <span>
#include <string_view>

#include "common/status/status.h"
#include "openxr_wrapper/graphics_plugin/graphics_plugin.h"
#include "openxr_wrapper/platform/platform.h"

namespace xrw {

class Instance {
  XrInstance _instance;

  Instance(XrInstance instance);

public:
  ~Instance();

  static ErrorOr<std::unique_ptr<Instance>> create(
      std::string_view engineName, const Platform& platform, const GraphicsPlugin& graphicsPlugin);

  XrInstance getXrInstance() const;
};

}  // namespace xrw
