#pragma once

#include <memory>
#include <openxr/openxr.h>

namespace xrw {

class Space {
  Space(XrSpace space);

public:
  static std::unique_ptr<Space> create(XrSession session, XrReferenceSpaceType type);

  static std::unique_ptr<Space> create(
      XrSession session, XrAction action, XrPosef poseInActionSpace, XrPath subactionPath = 0);

  ~Space();

  XrSpace getXrSpace() const;

private:
  XrSpace _space = XR_NULL_HANDLE;
};

}  // namespace xrw
