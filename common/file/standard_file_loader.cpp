#include "standard_file_loader.h"

#include "lib/buffer/buffer.h"

#include <fstream>
#include <string_view>

ErrorOr<lib::Buffer<std::byte>> StandardFileLoader::loadFile(
    std::string_view filePath) const {
  std::ifstream file(filePath.data(), std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    return Error(EngineError::LOAD_FAILURE);
  }

  const std::streampos fileSize = file.tellg();

  lib::Buffer<std::byte> buffer(fileSize);

  file.seekg(0);
  file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

  return buffer;
}
