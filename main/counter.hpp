#ifndef APPLICATION_COUNTER_HPP
#define APPLICATION_COUNTER_HPP
#pragma once

#include <cmath>
#include <limits>
#include <cstdint>
#include <functional>


#ifdef __cplusplus
namespace application {
namespace counter {

    void touch(bool reedSwitchState) noexcept(true);
    void init() noexcept(true);

namespace value {

    using Real = float;
    using Raw = ::std::uint32_t;

    constexpr static auto convert(Raw) noexcept(true);
    constexpr static auto convert(Real) noexcept(true);

    template <class> constexpr static auto max() noexcept(true);
    template <class = Raw> static auto get() noexcept(true);

    void set(Raw) noexcept(true);
    void set(Real) noexcept(true);

} // namespace value

namespace handler {

    struct Key;
    using Function = ::std::function<void(value::Raw)>;

    handler::Key const * remove(Key const *key) noexcept(true);
    handler::Key const * install(handler::Function &&handler) noexcept(true);
    handler::Key const * install(handler::Function const &handler) noexcept(true);

} // namespace handler

namespace value {

    inline constexpr static auto convert(Raw value) noexcept(true) {
        auto const temp = static_cast<Real>(value / 2);
        return +1.0e-2 * ((0 == (value & 1)) ? temp : (+3.0e-1 + temp));
    }

    template <class> struct Traits final {
    private:
        template <class ... T> Traits(T && ...) = delete;
        template <class T> Traits & operator = (T &&) = delete;
    };

    template <> struct Traits<Raw> final {
        inline constexpr static auto max() noexcept(true) { return ::std::numeric_limits<Raw>::max(); }
        static Raw get() noexcept(true);

    private:
        template <class ... T> Traits(T && ...) = delete;
        template <class T> Traits & operator = (T &&) = delete;
    };

    template <> struct Traits<Real> final {
        inline constexpr static auto max() noexcept(true) { return convert(Traits<Raw>::max()); }
        inline static auto get() noexcept(true) { return convert(Traits<Raw>::get()); }

    private:
        template <class ... T> Traits(T && ...) = delete;
        template <class T> Traits & operator = (T &&) = delete;
    };

    template <class T> inline constexpr static auto max() noexcept(true) { return Traits<T>::max(); }
    template <class T> inline static auto get() noexcept(true) { return Traits<T>::get(); }

    inline constexpr static auto convert(Real value) noexcept(true) {
        if (! (+0.0e+0 < value)) return static_cast<Raw>(0);
        if (! (max<Real>() > value)) return max<Raw>();
        auto const x1 = ::std::floor(value * 10);
        auto const x2 = ::std::floor(value * 100);
        return 2 * static_cast<Raw>(x2) + static_cast<Raw>((+3.0e-1 <= (x1 - x2)) ? 1 : 0);
    }

    inline void set(Real value) noexcept(true) { return set(convert(value)); }

} // namespace value
} // namespace counter
} // namespace application
#endif // __cplusplus

#endif // APPLICATION_COUNTER_HPP
