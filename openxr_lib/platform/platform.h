#pragma once

#include <openxr/openxr.h>

#include <span>
#include <string_view>

namespace oxr {

class Platform {
 public:
  virtual const XrBaseInStructure* getInstanceCreateExtension() const = 0;

  virtual std::span<const char* const> getInstanceExtensions() const = 0;

  virtual ~Platform() = default;
};

} // oxr
