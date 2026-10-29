#include "space.h"

#include "common/util/engine_exception.h"
#include "openxr_wrapper/util/check.h"

namespace xrw {

Space::Space(XrSpace space) : _space(space) {}

std::unique_ptr<Space> Space::create(XrSession session, XrReferenceSpaceType type) {
  if (session == XR_NULL_HANDLE) [[unlikely]] {
    throw EngineException("Cannot create XrSpace from XR_NULL_HANDLE XrSession.");
  }
  const XrReferenceSpaceCreateInfo referenceSpaceCreateInfo = {
    .type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
    .referenceSpaceType = type,
    .poseInReferenceSpace = {
                             .orientation = {0.0f, 0.0f, 0.0f, 1.0f}, .position = {0.0f, 0.0f, 0.0f}}
  };
  XrSpace space;
  CHECK_XRCMD(xrCreateReferenceSpace(session, &referenceSpaceCreateInfo, &space),
              "Failed to create XrSpace.");
  return std::unique_ptr<Space>(new Space(space));
}

std::unique_ptr<Space> Space::create(
    XrSession session, XrAction action, XrPosef poseInActionSpace, XrPath subactionPath) {
  if (session == XR_NULL_HANDLE) [[unlikely]] {
    throw EngineException("Cannot create XrSpace from XR_NULL_HANDLE XrSession.");
  }
  const XrActionSpaceCreateInfo createInfo{
    .type = XR_TYPE_ACTION_SPACE_CREATE_INFO,
    .action = action,
    .subactionPath = subactionPath,
    .poseInActionSpace = poseInActionSpace};
  XrSpace space;
  CHECK_XRCMD(xrCreateActionSpace(session, &createInfo, &space), "Failed to create XrSpace.");
  return std::unique_ptr<Space>(new Space(space));
}

Space::~Space() {
  if (_space != XR_NULL_HANDLE) {
    xrDestroySpace(_space);
  }
}

XrSpace Space::getXrSpace() const {
  return _space;
}

}  // namespace xrw
