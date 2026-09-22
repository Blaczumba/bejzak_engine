#pragma once

#include <string_view>

#include "common/ref/ref.h"
#include "common/util/resource_handles.h"
#include "vulkan/resource_manager/hasher.h"
#include "vulkan/wrapper/framebuffer/framebuffer.h"
#include "vulkan/wrapper/memory_allocator/allocation.h"
#include "vulkan/wrapper/memory_objects/buffer.h"
#include "vulkan/wrapper/memory_objects/image.h"
#include "vulkan/wrapper/sampler/sampler.h"

namespace {

template <typename T>
struct Handle;

template <>
struct Handle<Sampler> {
  using type = SamplerHandle;
  using underlying_resource = VkSampler;
  using metadata = SamplerMetadata;
  using hasher = SamplerHasher;
  static constexpr size_t size = MAX_SAMPLERS;
  static constexpr std::string_view name = "Sampler";
  static constexpr common::RefType erased_type = common::RefType::Sampler;
};

template <>
struct Handle<Buffer> {
  using type = BufferHandle;
  using underlying_resource = VkBuffer;
  using metadata = BufferMetadata;
  static constexpr size_t size = MAX_BUFFERS;
  static constexpr std::string_view name = "Buffer";
  static constexpr common::RefType erased_type = common::RefType::Buffer;
};

template <>
struct Handle<Image> {
  using type = ImageHandle;
  using underlying_resource = VkImage;
  using metadata = ImageMetadata;
  static constexpr size_t size = MAX_IMAGES;
  static constexpr std::string_view name = "Image";
  static constexpr common::RefType erased_type = common::RefType::Image;
};

template <>
struct Handle<VirtualAllocation> {
  using type = VirtualAllocationHandle;
  using underlying_resource = std::variant<VmaVirtualAllocation>;
  using metadata = VirtualAllocationMetadata;
  static constexpr size_t size = MAX_VIRTUAL_ALLOCATIONS;
  static constexpr std::string_view name = "VirtualAllocation";
  static constexpr common::RefType erased_type = common::RefType::VirtualAllocation;
};

template <>
struct Handle<Framebuffer> {
  using type = FramebufferHandle;
  using underlying_resource = VkFramebuffer;
  using metadata = FramebufferMetadata;
  static constexpr size_t size = MAX_FRAMEBUFFERS;
  static constexpr std::string_view name = "Framebuffer";
  static constexpr common::RefType erased_type = common::RefType::Framebuffer;
};

}  // namespace

template <typename T>
using HandleFor = typename Handle<T>::type;

template <typename T>
using UnderlyingResourceFor = typename Handle<T>::underlying_resource;

template <typename T>
using MetadataFor = typename Handle<T>::metadata;

template <typename T>
using HasherFor = typename Handle<T>::hasher;

template <typename T>
constexpr std::string_view NAME_OF = Handle<T>::name;

template <typename T>
constexpr size_t MAX_NUMBER_OF = Handle<T>::size;

template <typename T>
constexpr common::RefType ErasedTypeOf = Handle<T>::erased_type;
