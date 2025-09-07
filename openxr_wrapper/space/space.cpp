#include "space.h"

#include "openxr_wrapper/util/check.h"

namespace xrw {

Space::Space(XrSpace space) : _space(space) {}

ErrorOr<std::unique_ptr<Space>> Space::create(XrSession session, XrReferenceSpaceType type) {
  if (session == XR_NULL_HANDLE) {
    return Error(EngineError::NULLPTR_REFERENCE);
  }
  const XrReferenceSpaceCreateInfo referenceSpaceCreateInfo = {
    .type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
    .referenceSpaceType = type,
    .poseInReferenceSpace = {
                             .orientation = {0.0f, 0.0f, 0.0f, 1.0f}, .position = {0.0f, 0.0f, 0.0f}}
  };
  XrSpace space;
  CHECK_XRCMD(xrCreateReferenceSpace(session, &referenceSpaceCreateInfo, &space));
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
