#include <cstdint>
#include <numeric>
#include <vector>

#include "memory_objects/buffer.h"
#include "memory_objects/texture/texture.h"
#include "descriptor_set/descriptor_set.h"

enum class TextureHandle : uint16_t { INVALID = std::numeric_limits<uint16_t>::max() };
enum class BufferHandle : uint16_t { INVALID = std::numeric_limits<uint16_t>::max() };

class DescriptorBindingManager {
	std::vector<VkImageView> _textures;
	std::vector<VkBuffer> _buffers;

	const DescriptorSet& _descriptorSet;

public:
	DescriptorBindingManager(const DescriptorSet& descriptorSet);

	TextureHandle storeTexture(const Texture& texture);

	BufferHandle storeBuffer(const Buffer& buffer);

};