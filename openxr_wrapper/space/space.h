#pragma once

#include <memory>
#include <openxr/openxr.h>

#include "common/status/status.h"

namespace xrw {

class Space {
  Space(XrSpace space);

public:
  static ErrorOr<std::unique_ptr<Space>> create(XrSession session, XrReferenceSpaceType type);

  ~Space();

  XrSpace getXrSpace() const;

private:
  XrSpace _space = XR_NULL_HANDLE;
};

}  // namespace xrw
