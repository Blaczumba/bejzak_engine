#include "engine/system.h"

#include <cassert>
#include <memory>

namespace engine {

System::System(std::unique_ptr<FileLoader> fileLoader) noexcept
  : _fileLoader(std::move(fileLoader)) {}

const FileLoader& System::getFileLoader() const noexcept {
  assert(_fileLoader != nullptr && "FileLoader cannot be nullptr.");
  return *_fileLoader;
}

}  // namespace engine
