#include "file_loader.h"

#include "lib/buffer/buffer.h"

#include <cstddef>
#include <memory>
#include <string_view>

class StandardFileLoader : public FileLoader {
public:
  ErrorOr<lib::Buffer<std::byte>> loadFile(std::string_view filePath) const override;
};
