#include <list>
#include <mutex>
#include <cstdint>
#include <utility>
#include <type_traits>
#include <unordered_map>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>

#include <esp_log.h>
#include <esp_err.h>

#include "gpio.hpp"


namespace application {
namespace gpio {
namespace handler {

    struct Key {
        virtual ~Key() = default;

    protected:
        Key() = default;
        Key(Key &&) = default;
        Key & operator = (Key &&) = default;

    private:
        Key(Key const &) = delete;
        Key & operator = (Key const &) = delete;
    };

    struct Holder final : Key {
        Function function;

        Holder() = default;
        Holder(Holder &&) = default;
        Holder & operator = (Holder &&) = default;

        inline explicit Holder(Function &&function) noexcept(true) : function(::std::move(function)) {}
        inline explicit Holder(Function const &function) noexcept(true) : function(function) {}

    private:
        Holder(Holder const &) = delete;
        Holder & operator = (Holder const &) = delete;
    };

namespace helpers {

    template <class ... T> inline static auto disableUnusedWarning(T && ...) noexcept(true) {}

    template <class functionT, class sectionT> inline static Key const * install(functionT &&function, sectionT &section) noexcept(true) {
        if (! static_cast<bool>(function)) return nullptr;
        ::std::unique_lock<::std::decay_t<decltype(section.mutex)>> const lock{section.mutex};
        if (! static_cast<bool>(lock)) return nullptr;
        try { section.list.push_back(handler::Holder{::std::forward<functionT>(function)}); }
        catch(...) { return nullptr; }
        return &section.list.back();
    }

    template <class contextT, class ... T> inline static auto raise(contextT &context, T && ... payload) noexcept(true) {
        ::std::unique_lock<::std::decay_t<decltype(context.mutex)>> const lock{context.mutex};
        disableUnusedWarning(lock);
        for (auto const &holder : context.list) if (static_cast<bool>(holder.function)) try { holder.function(::std::forward<T>(payload) ...); } catch(...) {}
    }

} // namespace helpers
} // namespace handler

namespace global {
namespace constants {
namespace pins {

    constexpr static auto const POWER = ::GPIO_NUM_32;
    constexpr static auto const REED_SWITCH = ::GPIO_NUM_33;

} // namespace pins

    constexpr static auto const INTR_DEFAULT_MASK = 0;

} // namespace constants

    struct Object final {
        struct Handlers final {
            struct Section final {
                using List = ::std::list<handler::Holder>;
                List list;
                ::std::recursive_mutex mutex;
                ::TaskHandle_t task = nullptr;
            };

            Section power, reedSwitch;
            ::std::unordered_map<handler::Key const *, Section *> map;
        };

        Handlers handlers;

        inline static auto & instance() noexcept(true) { static Object instance; return instance; }

    private:
        Object() = default;
        template <class T> Object & operator = (T &&) = delete;
        template <class ... T> explicit Object(T && ...) = delete;
    };

} // namespace global

    using Global = global::Object;

namespace handler {
namespace isr {
namespace entries {

    static auto IRAM_ATTR power(void *) noexcept(true) {
        auto * const task = Global::instance().handlers.power.task;
        if (static_cast<bool>(task)) ::xTaskNotifyFromISR(task, 0, eNoAction, nullptr);
    }

    static auto IRAM_ATTR reedSwitch(void *) noexcept(true) {
        auto * const task = Global::instance().handlers.reedSwitch.task;
        if (static_cast<bool>(task)) ::xTaskNotifyFromISR(task, 0, eNoAction, nullptr);
    }

} // namespace entries

    template <class pinT, class handlerT> inline static auto install(pinT &&pin, handlerT &&handler) noexcept(true) {
        return ::gpio_isr_handler_add(pin, ::std::forward<handlerT>(handler), nullptr);
    }

} // namespace isr

namespace tasks {

    static auto power(void *) noexcept(true) {
        while(true) if (pdTRUE == ::xTaskNotifyWait(0, 0, nullptr, portMAX_DELAY))
            helpers::raise(Global::instance().handlers.power, entries::power::state());
    }

    static auto reedSwitch(void *) noexcept(true) {
        while(true) if (pdTRUE == ::xTaskNotifyWait(0, 0, nullptr, portMAX_DELAY))
            helpers::raise(Global::instance().handlers.reedSwitch, entries::reed_switch::state());
    }

} // namespace tasks

namespace helpers {

    inline static auto createTasks() noexcept(true) {
        auto &handlers = Global::instance().handlers;
        constexpr static auto stack = ::std::max(configMINIMAL_STACK_SIZE, 8192);
        constexpr static auto priority = configMAX_PRIORITIES - 1;
        auto status = ::xTaskCreate(handler::tasks::power, "app/gpio/power", stack, nullptr, priority, &handlers.power.task);
        if (pdTRUE != status) return ESP_FAIL;
        status = ::xTaskCreate(handler::tasks::reedSwitch, "app/gpio/reedSwitch", stack, nullptr, priority, &handlers.reedSwitch.task);
        if (pdTRUE == status) return ESP_OK;
        vTaskDelete(handlers.power.task);
        handlers.power.task = nullptr;
        return ESP_FAIL;
    }

} // namespace helpers
} // namespace handler

namespace entries {
namespace power {

    bool state() noexcept(true) { return 0 != ::gpio_get_level(global::constants::pins::POWER); }

namespace handler {

    Key const * install(Function &&handler) noexcept(true) {
        return gpio::handler::helpers::install(::std::move(handler), Global::instance().handlers.power);
    }

    Key const * install(Function const &handler) noexcept(true) {
        return gpio::handler::helpers::install(handler, Global::instance().handlers.power);
    }

} // namespace handler
} // namespace power

namespace reed_switch {

    bool state() noexcept(true) { return 0 != ::gpio_get_level(global::constants::pins::REED_SWITCH); }

namespace handler {

    Key const * install(Function &&handler) noexcept(true) {
        return gpio::handler::helpers::install(::std::move(handler), Global::instance().handlers.reedSwitch);
    }

    Key const * install(Function const &handler) noexcept(true) {
        return gpio::handler::helpers::install(handler, Global::instance().handlers.reedSwitch);
    }

} // namespace handler
} // namespace reed_switch
} // namespace entries

    void init() noexcept(true) {
        static auto flag = false;
        if (flag) return;
        flag = true;

        Global::instance();

        ESP_LOGI("app/gpio", "initialization started");

        ::gpio_config_t config;

        // interrupt of any edge
        config.intr_type = ::GPIO_INTR_ANYEDGE;
        // bit mask of the pins, use GPIO32 here
        config.pin_bit_mask = 1ULL << global::constants::pins::REED_SWITCH;
        // set as input mode
        config.mode = ::GPIO_MODE_INPUT;
        // enable pull-up mode
        config.pull_up_en = ::GPIO_PULLUP_ENABLE;
        // disable pull-down mode
        config.pull_down_en = ::GPIO_PULLDOWN_DISABLE;
        ESP_ERROR_CHECK(::gpio_config(&config));

        // interrupt of any edge
        config.intr_type = ::GPIO_INTR_ANYEDGE;
        // bit mask of the pins, use GPIO33 here
        config.pin_bit_mask = 1ULL << global::constants::pins::POWER;
        // set as input mode
        config.mode = ::GPIO_MODE_INPUT;
        // disable pull-up mode
        config.pull_up_en = ::GPIO_PULLUP_DISABLE;
        // enable pull-down mode
        config.pull_down_en = ::GPIO_PULLDOWN_ENABLE;
        ESP_ERROR_CHECK(::gpio_config(&config));

        // change gpio intrrupt type
        ESP_ERROR_CHECK(::gpio_set_intr_type(global::constants::pins::POWER, ::GPIO_INTR_ANYEDGE));
        ESP_ERROR_CHECK(::gpio_set_intr_type(global::constants::pins::REED_SWITCH, ::GPIO_INTR_ANYEDGE));

        // start gpio tasks
        ESP_ERROR_CHECK(handler::helpers::createTasks());

        // install gpio isr service
        ESP_ERROR_CHECK(::gpio_install_isr_service(global::constants::INTR_DEFAULT_MASK));

        // hook isr handler for specific gpio pin
        ESP_ERROR_CHECK(handler::isr::install(global::constants::pins::POWER, handler::isr::entries::power));

        // hook isr handler for specific gpio pin
        ESP_ERROR_CHECK(handler::isr::install(global::constants::pins::REED_SWITCH, handler::isr::entries::reedSwitch));

        ESP_LOGI("app/gpio", "initialization finished");
    }

} // namespace gpio
} // namespace application
