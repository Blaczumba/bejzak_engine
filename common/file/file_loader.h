#pragma once

#include <cstddef>
#include <string_view>

#include "common/status/status.h"
#include "lib/buffer/buffer.h"

class FileLoader {
public:
  virtual ErrorOr<lib::Buffer<std::byte>> loadFile(std::string_view filePath) const = 0;
};
