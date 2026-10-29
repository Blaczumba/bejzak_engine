#pragma once

#include <memory>
#include <openxr/openxr.h>
#include <span>
#include <string>
#include <string_view>
#include <unordered_set>

#include "openxr_wrapper/graphics_plugin/graphics_plugin.h"
#include "openxr_wrapper/platform/platform.h"

namespace xrw {

class Instance {
  Instance(XrInstance instance, std::unordered_set<std::string>&& extensions);

public:
  ~Instance();

  static std::unique_ptr<Instance> create(
      std::string_view engineName, const Platform& platform, const GraphicsPlugin& graphicsPlugin);

  XrInstance getXrInstance() const;

  bool isExtensionEnabled(std::string_view extensionName) const noexcept;

private:
  XrInstance _instance;

  std::unordered_set<std::string> _extensions;
};

}  // namespace xrw
