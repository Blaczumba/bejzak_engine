#pragma once

#include <string>

#include "common/model_loader/model_loader.h"
#include "common/status/status.h"

ErrorOr<VertexData> loadObj(std::string& data);
