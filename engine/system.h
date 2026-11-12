#pragma once

#include <memory>

#include "common/file/file_loader.h"

namespace engine {

class System {
public:
  System(std::unique_ptr<FileLoader> fileLoader) noexcept;

  const FileLoader& getFileLoader() const noexcept;

private:
  std::unique_ptr<FileLoader> _fileLoader;
};

}  // namespace engine
