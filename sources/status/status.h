#pragma once

#include <vulkan/vulkan.h>

#include <expected>
#include <string_view>
#include <variant>

enum class EngineError : uint32_t {
    INDEX_OUT_OF_RANGE,
    EMPTY_COLLECTION,
    SIZE_MISMATCH,
    NOT_RECOGNIZED_TYPE,
    NOT_FOUND,
    NOT_MAPPED,
    RESOURCE_EXHAUSTED,
    LOAD_FAILURE,
    FLAG_NOT_SPECIFIED
};

constexpr std::string_view engineErrorToString(EngineError error) {
    switch (error) {
        case EngineError::INDEX_OUT_OF_RANGE: return "Index out of range";
        case EngineError::EMPTY_COLLECTION: return "Empty collection";
        case EngineError::SIZE_MISMATCH: return "Size mismatch";
        case EngineError::NOT_RECOGNIZED_TYPE: return "Not recognized type";
        case EngineError::NOT_FOUND: return "Not found";
        case EngineError::NOT_MAPPED: return "Not mapped";
        case EngineError::RESOURCE_EXHAUSTED: return "Resource exhausted";
        case EngineError::LOAD_FAILURE: return "Load failure";
        case EngineError::FLAG_NOT_SPECIFIED: return "Flag not specified";
    }
    return "Unknown EngineError";
}

constexpr std::string_view vkResultToString(VkResult result) {
    switch (result) {
        case VK_SUCCESS: return "VK_SUCCESS";
        case VK_NOT_READY: return "VK_NOT_READY";
        case VK_TIMEOUT: return "VK_TIMEOUT";
        case VK_EVENT_SET: return "VK_EVENT_SET";
        case VK_EVENT_RESET: return "VK_EVENT_RESET";
        case VK_INCOMPLETE: return "VK_INCOMPLETE";
        case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
    }
    return "Unknown VkResult error code";
}

using ErrorType = std::variant<VkResult, EngineError>;

std::string_view errorToString(const ErrorType& error);

template<typename T>
using ErrorOr = std::expected<T, ErrorType>;

using Status = std::expected<void, ErrorType>;

struct StatusOk : public Status {
    StatusOk() : Status{} {}
};

using Error = std::unexpected<ErrorType>;
