#pragma once

#include <openxr/openxr.h>

#include "common/status/status.h"

#define CHECK_XRCMD(cmd) \
    if (XrResult result = cmd; result != XR_SUCCESS) return Error(result)