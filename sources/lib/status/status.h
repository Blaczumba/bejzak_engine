#pragma once

#include <expected>
#include <string>
#include <utility>

namespace lib {

template<typename T>
using ErrorOr = std::expected<T, std::string>;

using Status = std::expected<void, std::string>;

struct StatusOk : public Status {};

using Error = std::unexpected<std::string>;

}

// Helper to generate a unique variable name using __LINE__
#define CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) CONCAT_IMPL(x, y)
#define UNIQUE_NAME(base) CONCAT(base, __LINE__)

// #ifndef NDEBUG
#define ASSIGN_OR_RETURN(variable, status)                          \
        auto&& UNIQUE_NAME(_result_) = (status);                    \
        /* In Debug mode, perform error checking */                 \
        if (!UNIQUE_NAME(_result_).has_value()) [[unlikely]]        \
            return std::unexpected(UNIQUE_NAME(_result_).error());  \
        variable = std::move(UNIQUE_NAME(_result_).value())

#define RETURN_IF_ERROR(status)                         \
        auto&& UNIQUE_NAME(_result_) = (status);        \
        if (!UNIQUE_NAME(_result_)) [[unlikely]]        \
            return UNIQUE_NAME(_result_)
/*#else
    #define ASSIGN_OR_RETURN(variable, status)      \
        variable = (status.value())

    #define RETURN_IF_ERROR(status)                 \
        status
#endif  */                                                        