#include "session.h"

#include "common/status/status.h"
#include "openxr_wrapper/system/system.h"
#include "openxr_wrapper/util/check.h"

namespace xrw {

Session::Session(XrSession session, const System& system) : _session(session), _system(system) {}

ErrorOr<std::unique_ptr<Session>> Session::create(
    const System& system, GraphicsPlugin& graphicsPlugin) {
  const XrSessionCreateInfo create_info = {
    .type = XR_TYPE_SESSION_CREATE_INFO,
    .next = graphicsPlugin.getGraphicsBinding(),
    .systemId = system.getXrSystemId()};

  XrSession session;
  CHECK_XRCMD(xrCreateSession(system.getInstance().getXrInstance(), &create_info, &session));
  return std::unique_ptr<Session>(new Session(session, system));
}

const System& Session::getSystem() const {
  return _system;
}

}  // namespace xrw
