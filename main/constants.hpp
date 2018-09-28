#ifndef APPLICATION_CONSTANTS_HPP
#define APPLICATION_CONSTANTS_HPP
#pragma once

#include <cstdint>
#include <string>


#ifdef __cplusplus
namespace application {
namespace constants {

    constexpr static auto const MAX_SSID_LENGTH = 32;
    constexpr static auto const MAX_WIFI_PASSWORD_LENGTH = 64;

} // namespace constants
} // namespace application
#endif // __cplusplus

#endif // APPLICATION_CONSTANTS_HPP
