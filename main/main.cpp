#include <iomanip>
#include <utility>
#include <sstream>
#include <type_traits>

#include <esp_log.h>
#include <esp_system.h>

#include "gpio.hpp"
#include "wifi.hpp"
#include "counter.hpp"
#include "storage.hpp"
#include "chip_info.hpp"


extern "C" void app_main() {
    ESP_LOGI("app/main", "initialization started");

    ::application::chip_info::execute();
    ::application::gpio::init();
    ::application::counter::init();
    ::application::storage::init();
    ::application::wifi::init();

    auto &&storage = ::application::storage::Data{::application::storage::data::get()};

    if (storage.temporaryAccessPointSsid.empty()) {
        ::std::stringstream stream; stream << "homecounter-" << ::std::hex << ::std::setw(8) << ::std::setfill('0') << ::esp_random();
        storage.temporaryAccessPointSsid = stream.str();
    }

    ::application::counter::value::set(storage.counter);

    auto &&wifi = ::application::wifi::Settings{::application::wifi::settings::get()};
    wifi.temporaryAccessPointSsid = storage.temporaryAccessPointSsid;
    ::application::wifi::settings::set(::std::move(wifi));

    ::application::storage::data::set(::std::move(storage));

    ::application::gpio::entries::power::handler::install([] (auto &&state) {
        ESP_LOGI("app/main", "power %d", ::std::forward<decltype(state)>(state));
    });

    ::application::gpio::entries::reed_switch::handler::install([] (auto && ...) {
        ::application::counter::touch(! ::application::gpio::entries::reed_switch::state());
    });

    ::application::counter::handler::install([] (auto &&value) {
        auto &&data = ::application::storage::Data{::application::storage::data::get()};
        data.counter = ::std::forward<decltype(value)>(value);
        ::application::storage::data::set(::std::move(data));
        ESP_LOGI("app/main", "counter handler: %f", ::application::counter::value::convert(::std::forward<decltype(value)>(value)));
    });

    ESP_LOGI("app/main", "initialization finished");
}
