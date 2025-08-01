#include "system.h"

#include "common/status/status.h"

#include <openxr/openxr.h>

#include <memory>

namespace xrw {

System::System(XrSystemId systemId, const Instance& instance) : _systemId(systemId), _instance(instance) {}

ErrorOr<std::unique_ptr<System>> System::create(const Instance& instance) {
    XrSystemGetInfo systemInfo = {
        .type = XR_TYPE_SYSTEM_GET_INFO,
        .formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY,
    };

    XrSystemId systemId;
    if (XrResult result = xrGetSystem(instance.getXrInstance(), &systemInfo, &systemId); result != XR_SUCCESS) {
        return Error(result);
    }
    return std::unique_ptr<System>(new System(systemId, instance));
}

XrSystemId System::getXrSystemId() const {
    return _systemId;
}

} // xrw