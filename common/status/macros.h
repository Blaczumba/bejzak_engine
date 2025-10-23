#pragma once

#include <expected>
#include <utility>

// Helper to generate a unique variable name using __LINE__
#define CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) CONCAT_IMPL(x, y)
#define UNIQUE_NAME(base) CONCAT(base, __LINE__)

#define ASSIGN_OR_RETURN(variable, status)                      \
    auto&& UNIQUE_NAME(_result_) = (status);                    \
    if (!UNIQUE_NAME(_result_).has_value()) [[unlikely]]        \
        return std::unexpected(UNIQUE_NAME(_result_).error());  \
    variable = std::move(UNIQUE_NAME(_result_).value())

#define RETURN_IF_ERROR(status)                                                     \
    if (auto UNIQUE_NAME(_result_) = (status); !UNIQUE_NAME(_result_)) [[unlikely]] \
        return std::unexpected(UNIQUE_NAME(_result_).error())

#define UPDATE_STATUS(statusVar, status)               \
    if (auto UNIQUE_NAME(_result_) = (status); !UNIQUE_NAME(_result_)) [[unlikely]] \
        statusVar = std::unexpected(UNIQUE_NAME(_result_).error())
