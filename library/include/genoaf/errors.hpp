#pragma once

#include <format>
#include <stdexcept>

namespace genoaf
{
    class Exception : public std::runtime_error
    {
    public:
        using runtime_error::runtime_error;
    };
}

#define GENOAF_STRINGIFY_IMPL(x) #x
#define GENOAF_STRINGIFY(x) GENOAF_STRINGIFY_IMPL(x)
#define GENOAF_LINE GENOAF_STRINGIFY(__LINE__)

#define GENOAF_INVARIANT(invariant, message, ...)                                   \
    if (invariant) {                                                                \
    } else {                                                                        \
        throw ::genoaf::Exception(std::format(                                      \
            "Invariant failed at " __FILE__ ":" GENOAF_LINE "[" # invariant "]: "   \
            message, ##__VA_ARGS__));                                               \
    }

#define GENOAF_THROW(message, ...)                                                  \
    throw ::genoaf::Exception(std::format(                                          \
        "Exception thrown from " __FILE__ ":" GENOAF_LINE message, ##__VA_ARGS__));
