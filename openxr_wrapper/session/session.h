#pragma once

#include "common/status/status.h"
#include "openxr_wrapper/system/system.h"
#include "openxr_wrapper/platform/platform.h"

#include <openxr/openxr.h>

#include <memory>

namespace xrw {

class Session {
  XrSession _session;

  const System& _system;

  Session(XrSession session, const System& instance);

 public:
  ~Session() = default;

  static ErrorOr<std::unique_ptr<Session>> create(const System& instance, GraphicsPlugin& graphicsPlugin);

  XrSession getXrSession() const;

  const System& getSystem() const;
};

} // xrw