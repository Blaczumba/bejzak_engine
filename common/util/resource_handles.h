#pragma once

#include "lib/types/strong_int.h"
#include "lib/types/util.h"

// Bindless descriptor set indexing.
constexpr size_t MAX_UNIFORM_RESOURCES = 256;
DEFINE_STRONG_INT(UniformBufferHandle, lib::SmallestIndex<MAX_UNIFORM_RESOURCES>::type);
DEFINE_STRONG_INT(UniformTextureHandle, lib::SmallestIndex<MAX_UNIFORM_RESOURCES>::type);

constexpr size_t MAX_BUFFERS = 512;
DEFINE_STRONG_INT(BufferHandle, lib::SmallestIndex<MAX_BUFFERS>::type);

constexpr size_t MAX_IMAGES = 512;
DEFINE_STRONG_INT(ImageHandle, lib::SmallestIndex<MAX_IMAGES>::type);

constexpr size_t MAX_SAMPLERS = 128;
DEFINE_STRONG_INT(SamplerHandle, lib::SmallestIndex<MAX_SAMPLERS>::type);

constexpr size_t MAX_FRAMEBUFFERS = 32;
DEFINE_STRONG_INT(FramebufferHandle, lib::SmallestIndex<MAX_FRAMEBUFFERS>::type);

constexpr size_t MAX_RENDERPASSES = 32;
DEFINE_STRONG_INT(RenderpassHandle, lib::SmallestIndex<MAX_RENDERPASSES>::type);

constexpr size_t MAX_PIPELINES = 32;
DEFINE_STRONG_INT(PipelineHandle, lib::SmallestIndex<MAX_PIPELINES>::type);

constexpr size_t MAX_VIRTUAL_ALLOCATIONS = 128;
DEFINE_STRONG_INT(VirtualAllocationHandle, lib::SmallestIndex<MAX_VIRTUAL_ALLOCATIONS>::type);
