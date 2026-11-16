#pragma once

#include <boost/core/demangle.hpp>

#include <format>
#include <limits>
#include <string>
#include <vector>

namespace genoaf
{
    template<typename T>
    auto demangle()
    {
        return boost::core::demangle(
            typeid(std::decay_t<T>).name()
        );
    }

    constexpr static size_t kNoIndex = std::numeric_limits<size_t>::max();

    // NOTE: Empty Path points to the root
    using Path = std::vector<std::string>;
}

// TODO: erase it after migration to GCC-15
#if !defined(__cpp_lib_format_ranges)
    template <typename T>
    struct std::formatter<std::vector<T>> {
        constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

        auto format(const std::vector<T>& vec, std::format_context& ctx) const
        {
            auto out = ctx.out();
            *out++ = '[';
            for (size_t i = 0; i < vec.size(); ++i)
            {
                if (i > 0)
                {
                    out = std::format_to(out, ", ");
                }

                if constexpr (std::is_convertible_v<T, std::string_view>)
                {
                    out = std::format_to(out, "\"{}\"", vec[i]);
                }
                else
                {
                    out = std::format_to(out, "{}", vec[i]);
                }
            }
            *out++ = ']';
            return out;
        }
    };
#endif

#define GENOAF_NO_COPY(Cls)                     \
    Cls(Cls const &) = delete;                  \
    Cls & operator=(Cls const &) = delete;

#define GENOAF_NO_MOVE(Cls);                    \
    Cls(Cls &&) = delete;                       \
    Cls & operator=(Cls &&) = delete;

#define GENOAF_SPTR(Cls);                       \
    using Sptr = std::shared_ptr<Cls>;          \
    using CSptr = std::shared_ptr<Cls const>;   \
    using Wptr = std::weak_ptr<Cls>;            \
    using CWptr = std::weak_ptr<Cls const>;
