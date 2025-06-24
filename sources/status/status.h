#pragma once

#include <vulkan/vulkan.h>

#include <expected>
#include <variant>

enum class EngineError : uint32_t {
    INDEX_OUT_OF_RANGE,
    EMPTY_COLLECTION,
    SIZE_MISMATCH,
    NOT_RECOGNIZED_TYPE,
    NOT_FOUND,
    NOT_MAPPED,
    RESOURCE_EXHAUSTED,
    NOT_SUPPORTED_FRAMEBUFFER_IMAGE_LAYOUT,
    NOT_SUPPORTED_VALIDATION_LAYERS,
    LOAD_FAILURE,
    FLAG_NOT_SPECIFIED
};

using ErrorType = std::variant<VkResult, EngineError>;

template<typename T>
using ErrorOr = std::expected<T, ErrorType>;

using Status = std::expected<void, ErrorType>;

struct StatusOk : public Status {
    StatusOk() : Status{} {}
};

using Error = std::unexpected<ErrorType>;
