#include "image_loader.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

#include <cmath>
#include <stdexcept>
#include <vector>

ErrorOr<ImageResource> ImageLoader::load2DImage(std::string_view imagePath) {
    int width, height, channels;
    stbi_uc* pixels = stbi_load(imagePath.data(), &width, &height, &channels, STBI_rgb_alpha);
    if (!pixels) {
        return Error(EngineError::LOAD_FAILURE);
    }

    return ImageResource{
		.libraryResource = pixels,
		.dimensions = ImageDimensions {
			.width = static_cast<uint32_t>(width),
			.height = static_cast<uint32_t>(height),
			.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1, 
			.layerCount = 1,
			.copyRegions = {
				VkBufferImageCopy {
					.imageSubresource = {
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						.layerCount = 1
					},
					.imageExtent = {
						.width = static_cast<uint32_t>(width),
						.height = static_cast<uint32_t>(height),
						.depth = 1
					},
				}
			}
		},
        .data = pixels,
        .size = static_cast<uint32_t>(4 * width * height)
    };
}

ErrorOr<ImageResource> ImageLoader::loadCubemapImage(std::string_view imagePath) {
	ktxTexture* ktxTexture;
	if (ktxResult result = ktxTexture_CreateFromNamedFile(imagePath.data(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture); result != KTX_SUCCESS) {
		return Error(EngineError::LOAD_FAILURE);
	}

	ImageResource image{
		.libraryResource = ktxTexture,
		.dimensions = ImageDimensions {
			.width = ktxTexture->baseWidth,
			.height = ktxTexture->baseHeight,
			.mipLevels = ktxTexture->numLevels,
			.layerCount = 6
		},
		.data = ktxTexture->pData,
		.size = ktxTexture->dataSize
	};

	for (uint32_t face = 0; face < image.dimensions.layerCount; ++face) {
		for (uint32_t level = 0; level < image.dimensions.mipLevels; ++level) {
			// Calculate offset into staging buffer for the current mip level and face
			ktx_size_t offset;
			if (ktxResult result = ktxTexture_GetImageOffset(ktxTexture, level, 0, face, &offset); result != KTX_SUCCESS) {
				ktxTexture_Destroy(ktxTexture);
				return Error(EngineError::LOAD_FAILURE);
			}

			image.dimensions.copyRegions.emplace_back(
				VkBufferImageCopy {
					.bufferOffset = offset,
					.imageSubresource = {
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						.mipLevel = level,
						.baseArrayLayer = face,
						.layerCount = 1
					},
					.imageExtent = {
						.width = image.dimensions.width >> level,
						.height = image.dimensions.height >> level,
						.depth = 1
					},
				}
			);
		}
	}
	return image;
}

namespace {

struct Deallocator {
	void operator()(ktxTexture* texture) {
		ktxTexture_Destroy(texture);
	}

	void operator()(stbi_uc* texture) {
		stbi_image_free(texture);
	}

	void operator()(auto&&) {
		throw std::runtime_error("Unrecognized type of library image resource to free");
	}
};

}

void ImageLoader::deallocateResources(ImageResource& resource) {
	std::visit(Deallocator{}, resource.libraryResource);
}
