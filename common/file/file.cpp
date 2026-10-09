#include "file.h"

#include <format>
#include <stdexcept>
#include <string>
#include <string_view>

std::string_view getImageFileExtension(std::string_view filePath) {
  size_t lastDot = filePath.find_last_of(".");
  if (lastDot == std::string_view::npos) [[unlikely]] {
    throw std::runtime_error(std::format("File {} does not contain an extension.", filePath));
  }

  return filePath.substr(lastDot);
}

std::string joinPaths(std::string_view first, std::string_view second) {
  std::string result;
  result.reserve(first.size() + 1 + second.size());
  result.append(first);
  result.append("/");
  result.append(second);
  return result;
}
