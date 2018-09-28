#include <cmath>

#include <list>
#include <mutex>
#include <utility>
#include <type_traits>
#include <unordered_map>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include <esp_err.h>
#include <esp_log.h>

#include "counter.hpp"


namespace application {
namespace counter {
namespace helpers {

    template <class ... T> inline static auto disableUnusedWarning(T && ...) noexcept(true) {}

} // namespace helpers

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

    using namespace counter::helpers;

    template <class functionT, class sectionT> inline static Key const * install(functionT &&function, sectionT &section) noexcept(true) {
        if (! static_cast<bool>(function)) return nullptr;
        ::std::unique_lock<::std::decay_t<decltype(section.mutex)>> const lock{section.mutex, ::std::try_to_lock_t{}};
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

    struct Object final {
        struct Handlers final {
            using List = ::std::list<handler::Holder>;
            List list;
            ::std::recursive_mutex mutex;
        };

        Handlers handlers;
        value::Raw value = 0;
        ::std::recursive_mutex mutex;
        ::EventGroupHandle_t events = nullptr;

        inline static auto & instance() noexcept(true) { static Object instance; return instance; }

    private:
        Object() = default;
        template <class T> Object & operator = (T &&) = delete;
        template <class ... T> explicit Object(T && ...) = delete;
    };

} // namespace global

    using Global = global::Object;

namespace task {

    static auto main(void *) noexcept(true) {
        auto state = false;
        auto delay = portMAX_DELAY;
        auto &global = Global::instance();

        while(true) {
            auto const bits = ::xEventGroupWaitBits(global.events, 3, pdTRUE, pdFALSE, delay);

            if (0 != (1 & bits)) {
                delay = portMAX_DELAY;
                if (0 != (2 & bits)) ::xEventGroupSetBits(global.events, 2);
                auto const value = value::get();
                ESP_LOGD("app/counter", "value updated to %f, rising handlers", value::convert(value));
                handler::helpers::raise(global.handlers, value::get());
            }

            else if (0 != (2 & bits)) {
                if (state) ESP_LOGD("app/counter", "%s", "anti-bounce test failed, increment will be canceled");
                ::std::unique_lock<::std::decay_t<decltype(global.mutex)>> const lock{global.mutex};
                helpers::disableUnusedWarning(lock);
                state = (0 == (1 & global.value)) != (0 == (4 & bits));
                delay = state ? (((0 == (4 & bits)) ? 700 : 300) / portTICK_PERIOD_MS) : portMAX_DELAY;
            }

            else if (state) {
                state = false;
                delay = portMAX_DELAY;
                ::std::unique_lock<::std::decay_t<decltype(global.mutex)>> const lock{global.mutex};
                helpers::disableUnusedWarning(lock);
                if ((0 == (1 & global.value)) != (0 == (4 & bits))) {
                    global.value++; ::xEventGroupSetBits(global.events, 1);
                    ESP_LOGD("app/counter", "anti-bounce test passed");
                }
            }
        }
    }

} // namespace task

namespace value {

    Raw Traits<Raw>::get() noexcept(true) {
        auto &global = Global::instance();
        ::std::unique_lock<::std::decay_t<decltype(global.mutex)>> const lock{global.mutex};
        helpers::disableUnusedWarning(lock);
        return global.value;
    }

    void set(Raw value) noexcept(true) {
        auto &global = Global::instance();
        ::std::unique_lock<::std::decay_t<decltype(global.mutex)>> const lock{global.mutex};
        helpers::disableUnusedWarning(lock);
        if (value == global.value) return;
        global.value = value;
        ::xEventGroupSetBits(global.events, 1);
    }

} // namespace value

namespace handler {

    Key const * install(Function &&handler) noexcept(true) { return helpers::install(::std::move(handler), Global::instance().handlers); }
    Key const * install(Function const &handler) noexcept(true) { return helpers::install(handler, Global::instance().handlers); }

} // namespace handler

    void touch(bool reedSwitchState) noexcept(true) {
        auto &global = Global::instance();
        ::xEventGroupClearBits(global.events, 6);
        ::xEventGroupSetBits(global.events, reedSwitchState ? 6 : 2);
    }

    void init() noexcept(true) {
        static auto flag = false;
        if (flag) return;
        flag = true;

        ESP_LOGI("app/counter", "initialization started");
        auto &global = Global::instance();
        global.events = ::xEventGroupCreate();
        auto status = ::xTaskCreate(task::main, "app/counter", ::std::max(configMINIMAL_STACK_SIZE, 8192), nullptr, 0, nullptr);
        ESP_ERROR_CHECK(pdTRUE == status ? ESP_OK : ESP_FAIL);
        ESP_LOGI("app/counter", "app/counter task started");
        ESP_LOGI("app/counter", "initialization finished");
    }

} // namespace counter
} // namespace application
