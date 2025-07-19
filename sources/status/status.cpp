#include "status.h"

std::string_view errorToString(const ErrorType& error) {
    if (std::holds_alternative<VkResult>(error)) {
        return vkResultToString(std::get<VkResult>(error));
    }
    else {
        return engineErrorToString(std::get<EngineError>(error));
    }
}