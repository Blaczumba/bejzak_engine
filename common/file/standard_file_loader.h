#pragma once

#include <cstddef>
#include <memory>
#include <sstream>
#include <string_view>

#include "file_loader.h"
#include "lib/buffer/buffer.h"

class StandardFileLoader : public FileLoader {
public:
  ErrorOr<lib::Buffer<std::byte>> loadFileToBuffer(std::string_view filePath) const override;

  ErrorOr<std::istringstream> loadFileToStringStream(std::string_view filePath) const override;

  ~StandardFileLoader() override = default;
};
