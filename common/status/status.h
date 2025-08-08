#pragma once

#include <cstdint>
#include <expected>
#include <variant>

#include "macros.h"

enum class EngineError : uint32_t {
  INDEX_OUT_OF_RANGE,
  EMPTY_COLLECTION,
  SIZE_MISMATCH,
  NOT_RECOGNIZED_TYPE,
  NOT_FOUND,
  NOT_MAPPED,
  RESOURCE_EXHAUSTED,
  LOAD_FAILURE,
  FLAG_NOT_SPECIFIED,
  NULLPTR_REFERENCE,
  ALREADY_INITIALIZED
};

// Holds either an integer error code (VkResult, HRESULT) or an EngineError.
using ErrorType = std::variant<int, EngineError>;

template <typename T>
using ErrorOr = std::expected<T, ErrorType>;

using Status = std::expected<void, ErrorType>;

struct StatusOk : public Status {
  StatusOk() : Status{} {}
};

using Error = std::unexpected<ErrorType>;
