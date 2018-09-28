#ifndef APPLICATION_WIFI_HPP
#define APPLICATION_WIFI_HPP
#pragma once

#include <string>


#ifdef __cplusplus
namespace application {
namespace wifi {
namespace settings {

    struct Object final {
        struct Station final {
            bool enabled = false;
            ::std::string ssid;
            ::std::string password;
        };

        Station station;
        ::std::string temporaryAccessPointSsid;
    };

    Object const & get() noexcept(true);
    void set(Object &&settings) noexcept(true);
    void set(Object const &settings) noexcept(true);

} // namespace settings

    using Settings = settings::Object;

    void enable() noexcept(true);
    void disable() noexcept(true);
    void enableTemporaryAccessPoint() noexcept(true);

    void init() noexcept(true);

} // namespace wifi
} // namespace application
#endif // __cplusplus

#endif // APPLICATION_WIFI_HPP
