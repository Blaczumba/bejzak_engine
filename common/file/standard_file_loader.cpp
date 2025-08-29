#include "standard_file_loader.h"

#include <fstream>
#include <string_view>

#include "lib/buffer/buffer.h"

ErrorOr<lib::Buffer<std::byte>> StandardFileLoader::loadFileToBuffer(
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

ErrorOr<std::istringstream> StandardFileLoader::loadFileToStringStream(
    std::string_view filePath) const {
  // TODO: write directly to std::string without copying.
  ASSIGN_OR_RETURN(const lib::Buffer<std::byte> buffer, loadFileToBuffer(filePath));
  return std::istringstream(std::string(reinterpret_cast<const char*>(buffer.cbegin()),
                                        reinterpret_cast<const char*>(buffer.cend())));
}
