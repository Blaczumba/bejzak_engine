#pragma once

#include <memory>
#include <openxr/openxr.h>

#include "common/status/status.h"
#include "openxr_wrapper/instance/instance.h"

namespace xrw {

class System {
  XrSystemId _systemId;

  const Instance& _instance;

  System(XrSystemId systemId, const Instance& instance);

public:
  ~System() = default;

  static ErrorOr<std::unique_ptr<System>> create(const Instance& instance);

  XrSystemId getXrSystemId() const;

  const Instance& getInstance() const;
};

}  // namespace xrw
