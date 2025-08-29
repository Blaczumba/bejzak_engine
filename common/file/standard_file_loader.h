#pragma once

#include "file_loader.h"

#include "lib/buffer/buffer.h"

#include <cstddef>
#include <sstream>
#include <memory>
#include <string_view>

class StandardFileLoader : public FileLoader {
public:
  ErrorOr<lib::Buffer<std::byte>> loadFileToBuffer(std::string_view filePath) const override;

  ErrorOr<std::istringstream> loadFileToStringStream(std::string_view filePath) const override;

  ~StandardFileLoader() override = default;

};
