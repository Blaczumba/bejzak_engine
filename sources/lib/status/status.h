#pragma once

#include <expected>
#include <string_view>
#include <utility>

template<typename T>
using ErrorOr = std::expected<T, std::string_view>;

using Status = std::expected<void, std::string_view>;

#define RETURN_IF_ERROR(variable) \
    if (!(variable).has_value())  \
        return (variable).error();


// Helper to generate a unique variable name using __LINE__
#define CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) CONCAT_IMPL(x, y)
#define UNIQUE_NAME(base) CONCAT(base, __LINE__)

// Assigns the result of 'status' to 'variable' or returns an error
#define ASSIGN_OR_RETURN(variable, status)                      \
    auto&& UNIQUE_NAME(_result_) = (status);                    \
    if (!UNIQUE_NAME(_result_).has_value())                     \
        return std::unexpected(UNIQUE_NAME(_result_).error());  \
    variable = UNIQUE_NAME(_result_).value();