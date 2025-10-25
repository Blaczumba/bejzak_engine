#pragma once

#include <cstddef>
#include <string>
#include <string_view>

#include "common/status/status.h"
#include "lib/buffer/buffer.h"

class FileLoader {
public:
  virtual ErrorOr<lib::Buffer<std::byte>> loadFileToBuffer(std::string_view filePath) const = 0;

  virtual ErrorOr<std::string> loadFileToString(std::string_view filePath) const = 0;

  virtual ~FileLoader() = default;
};
