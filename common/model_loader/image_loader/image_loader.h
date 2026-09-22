#pragma once

#include <span>
#include <string_view>
#include <tuple>

#include "common/model_loader/image_loader/types.h"

std::tuple<ImageResource, OwnedImageData> loadImageStbi(std::span<const std::byte> imageData);

std::tuple<ImageResource, OwnedImageData> loadImageKtx(std::span<const std::byte> imageData);

std::tuple<ImageResource, OwnedImageData> loadImage(
    std::span<const std::byte> imageData, std::string_view filePath);
