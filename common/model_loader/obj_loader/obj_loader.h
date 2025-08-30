#pragma once

#include <sstream>

#include "common/status/status.h"
#include "common/model_loader/model_loader.h"

ErrorOr<VertexData> loadObj(std::istringstream& data);
