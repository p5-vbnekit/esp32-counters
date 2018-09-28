#ifndef APPLICATION_GPIO_HPP
#define APPLICATION_GPIO_HPP
#pragma once

#include <functional>


#ifdef __cplusplus
namespace application {
namespace gpio {
namespace handler {

    struct Key;
    using Function = ::std::function<void(bool)>;

    bool remove(Key const *key) noexcept(true);

} // namespace handler

namespace entries {
namespace power {

    bool state() noexcept(true);

namespace handler {

    using Key = gpio::handler::Key;
    using Function = gpio::handler::Function;

    bool remove(Key const *key) noexcept(true);
    Key const * install(Function &&handler) noexcept(true);
    Key const * install(Function const &handler) noexcept(true);

} // namespace handler
} // namespace power

namespace reed_switch {

    bool state() noexcept(true);

namespace handler {

    using Key = gpio::handler::Key;
    using Function = gpio::handler::Function;

    bool remove(Key const *key) noexcept(true);
    Key const * install(Function &&handler) noexcept(true);
    Key const * install(Function const &handler) noexcept(true);

} // namespace handler
} // namespace reed_switch
} // namespace entries

    void init() noexcept(true);

} // namespace gpio
} // namespace application
#endif // __cplusplus

#endif // APPLICATION_GPIO_HPP
