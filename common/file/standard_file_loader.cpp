#include "standard_file_loader.h"

#include <format>
#include <fstream>
#include <stdexcept>
#include <string_view>

#include "lib/buffer/buffer.h"

lib::Buffer<std::byte> StandardFileLoader::loadFileToBuffer(std::string_view filePath) const {
  std::ifstream file(filePath.data(), std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error(std::format("Failed to load {}", filePath));
  }

  const std::streampos fileSize = file.tellg();

  lib::Buffer<std::byte> buffer(fileSize);

  file.seekg(0);
  file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

  return buffer;
}

std::string StandardFileLoader::loadFileToString(std::string_view filePath) const {
  std::ifstream file(filePath.data(), std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error(std::format("Failed to load {}", filePath));
  }

  const std::streampos fileSize = file.tellg();

  std::string buffer;
  buffer.resize(fileSize);

  file.seekg(0);
  file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

  return buffer;
}
