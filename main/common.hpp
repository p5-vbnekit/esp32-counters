#ifndef APPLICATION_COMMON_HPP
#define APPLICATION_COMMON_HPP
#pragma once

#include <cstdint>
#include <string>


#ifdef __cplusplus
namespace application {
namespace common {
namespace wifi {

    constexpr static auto const MAX_SSID_LENGTH = 32;
    constexpr static auto const MAX_PASSWORD_LENGTH = 64;

} // namespace wifi
} // namespace common
} // namespace application
#endif // __cplusplus

#endif // APPLICATION_COMMON_HPP
