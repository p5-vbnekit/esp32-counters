#ifndef APPLICATION_STORAGE_HPP
#define APPLICATION_STORAGE_HPP
#pragma once

#include <cstdint>
#include <string>


#ifdef __cplusplus
namespace application {
namespace storage {
namespace data {

    using Counter = ::std::uint32_t;

    struct Object final {
        using Counter = data::Counter;

        Counter counter = 0;
        ::std::string temporaryAccessPointSsid;

        inline auto operator == (Object const &other) const noexcept(true) {
            return counter == other.counter && temporaryAccessPointSsid == other.temporaryAccessPointSsid;
        }

        inline auto operator != (Object const &other) const noexcept(true) { return ! (*this == other); }
    };

    Object const & get() noexcept(true);
    void set(Object &&data) noexcept(true);
    void set(Object const &data) noexcept(true);

} // namespace data

    using Data = data::Object;

    void init() noexcept(true);

} // namespace storage
} // namespace application
#endif // __cplusplus

#endif // APPLICATION_STORAGE_HPP
