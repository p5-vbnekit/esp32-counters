#include <cstring>
#include <cstdint>

#include <list>
#include <limits>
#include <vector>
#include <iomanip>
#include <sstream>
#include <exception>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>

#include <esp_err.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_system.h>
#include <esp_event_loop.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "wifi.hpp"
#include "common.hpp"


namespace application {
namespace wifi {
namespace helpers {

    inline static ::std::size_t safeStrLen(char const *str, ::std::size_t max) noexcept(true) {
        if (! static_cast<bool>(str)) return 0;
        for (::std::size_t i = 0; i < max; i++) if (0 == str[i]) return i;
        return max;
    }

    template <class T> inline static auto const * secondaryChannelToString(T &&reason) noexcept(true) {
        static ::std::unordered_map<::std::underlying_type_t<::std::decay_t<decltype(::WIFI_SECOND_CHAN_NONE)>>, char const *> const map {
            { ::WIFI_SECOND_CHAN_NONE, "HT20" },
            { ::WIFI_SECOND_CHAN_ABOVE, "above HT40" },
            { ::WIFI_SECOND_CHAN_BELOW, "below HT40" }
        };

        auto const iterator = map.find(::std::forward<T>(reason));
        return map.end() == iterator ? "unknown" : iterator->second;
    }

    template <class T> inline static auto const * authModeToString(T &&mode) noexcept(true) {
        static ::std::unordered_map<::std::underlying_type_t<::std::decay_t<decltype(::WIFI_AUTH_OPEN)>>, char const *> const map {
            { ::WIFI_AUTH_OPEN, "open" },
            { ::WIFI_AUTH_WEP, "WEP" },
            { ::WIFI_AUTH_WPA_PSK, "WPA PSK" },
            { ::WIFI_AUTH_WPA2_PSK, "WPA2 PSK" },
            { ::WIFI_AUTH_WPA_WPA2_PSK, "WPA & WPA2 PSK" },
            { ::WIFI_AUTH_WPA2_ENTERPRISE, "WPA2 ENTERPRISE" }
        };

        auto const iterator = map.find(::std::forward<T>(mode));
        return map.end() == iterator ? "unknown" : iterator->second;
    }

    template <class T> inline static auto const * chipherToString(T &&mode) noexcept(true) {
        static ::std::unordered_map<::std::underlying_type_t<::std::decay_t<decltype(::WIFI_CIPHER_TYPE_NONE)>>, char const *> const map {
            { ::WIFI_CIPHER_TYPE_NONE, "none" },
            { ::WIFI_CIPHER_TYPE_WEP40, "WEP40" },
            { ::WIFI_CIPHER_TYPE_WEP104, "WEP104" },
            { ::WIFI_CIPHER_TYPE_TKIP, "TKIP" },
            { ::WIFI_CIPHER_TYPE_CCMP, "CCMP" },
            { ::WIFI_CIPHER_TYPE_TKIP_CCMP, "TKIP & CCMP" }
        };

        auto const iterator = map.find(::std::forward<T>(mode));
        return map.end() == iterator ? "unknown" : iterator->second;
    }

    template <class T> inline static auto const * antennaToString(T &&mode) noexcept(true) {
        static ::std::unordered_map<::std::underlying_type_t<::std::decay_t<decltype(::WIFI_ANT_ANT0)>>, char const *> const map {
            { ::WIFI_ANT_ANT0, "1" },
            { ::WIFI_ANT_ANT1, "2" }
        };

        auto const iterator = map.find(::std::forward<T>(mode));
        return map.end() == iterator ? "unknown" : iterator->second;
    }

    inline static auto const * getInitConfigPointer() noexcept(true) {
        static ::wifi_init_config_t const config = WIFI_INIT_CONFIG_DEFAULT();
        return &config;
    }

    static ::esp_err_t eventHandler(void *, ::system_event_t *) { return ESP_OK; }

    template <class T> inline static auto makeTextFromEspError(T &&error) noexcept(false) {
        ::std::stringstream stream; stream << "esp error #" << +::std::forward<T>(error) << ": " << ::esp_err_to_name(error);
        return stream.str();
    }

    inline static auto scan() noexcept(false) {
        ::wifi_scan_config_t config;

        config.ssid = nullptr;
        config.bssid = nullptr;
        config.channel = 0;
        config.show_hidden = true;
        config.scan_type = ::WIFI_SCAN_TYPE_PASSIVE;
        config.scan_time.passive = 1000;

        auto error = ESP_OK;
        auto count = static_cast<::std::uint16_t>(0);
        ::std::vector<::wifi_ap_record_t> vector(4);
        auto &&list = ::std::list<::wifi_ap_record_t>{};

        while(config.channel < 14) {
            error = ::esp_wifi_scan_start(&config, true);
            switch (error) {
            default: throw ::std::runtime_error{"esp_wifi_scan_start(): " + makeTextFromEspError(error)};
            case ESP_ERR_WIFI_TIMEOUT: continue;
            case ESP_OK: break;
            }
            error = ::esp_wifi_scan_get_ap_num(&count);
            if (ESP_OK != error) throw ::std::runtime_error{"esp_wifi_scan_get_ap_num(): " + makeTextFromEspError(error)};
            if (0 < count) {
                if (vector.size() < count) vector.resize(count);
                error = ::esp_wifi_scan_get_ap_records(&count, vector.data());
                if (ESP_OK != error) throw ::std::runtime_error{"esp_wifi_scan_get_ap_records(): " + makeTextFromEspError(error)};
                if (0 < count) list.insert(list.end(), vector.begin(), vector.begin() + count);
            }
            config.channel++;
        }

        return ::std::move(list);
    }

} // namespace helpers

namespace tasks {

    void scanner(void *) noexcept(true) {
        while(true) {
            auto delay = 30000 / portTICK_PERIOD_MS;

            try {
                for (auto const &r: helpers::scan()) {
                    ::std::stringstream stream;

                    stream << ::std::hex;
                    stream << "access point, MAC = "
                           << +r.bssid[0] << ":" << +r.bssid[1] << ":" << +r.bssid[2] << ":"
                           << +r.bssid[3] << ":" << +r.bssid[4] << ":" << +r.bssid[5] << ", ";
                    stream << ::std::dec;

                    stream << "SSID = '" << ::std::string{
                        reinterpret_cast<char const *>(r.ssid), helpers::safeStrLen(reinterpret_cast<char const *>(r.ssid), sizeof(r.ssid))
                    } << "', ";

                    stream << "primary channel = " << +r.primary << ", ";
                    stream << "secondary channel = " << helpers::secondaryChannelToString(r.second) << ", ";
                    stream << "signal strength = " << +r.rssi << ", ";
                    stream << "auth mode = " << helpers::authModeToString(r.authmode) << ", ";
                    stream << "pairwise cipher = " << helpers::chipherToString(r.pairwise_cipher) << ", ";
                    stream << "group cipher = " << helpers::chipherToString(r.group_cipher) << ", ";
                    stream << "antenna = " << helpers::antennaToString(r.ant) << ", ";

                    stream << "mode = "; auto bgn = false;
                    if (0 != r.phy_11b) { bgn = true; stream << "b"; }
                    if (0 != r.phy_11g) { if (bgn) stream << "/"; bgn = true; stream << "g"; }
                    if (0 != r.phy_11n) { if (bgn) stream << "/"; bgn = true; stream << "n"; }
                    if (0 != r.phy_lr) { if (bgn) stream << "+"; bgn = true; stream << "low rate"; }
                    if (0 != r.wps) { if (bgn) stream << "+"; bgn = true; stream << "wps"; }
                    stream << ", ";

                    stream << "country = '" << ::std::string{
                        reinterpret_cast<char const *>(r.country.cc),
                        helpers::safeStrLen(reinterpret_cast<char const *>(r.country.cc), sizeof(r.country.cc))
                    } << "':";

                    stream << +r.country.schan << ":" << +r.country.nchan << ":" << +r.country.max_tx_power;
                    switch (r.country.policy) {
                    default: break;
                    case ::WIFI_COUNTRY_POLICY_AUTO: stream << ":auto"; break;
                    case ::WIFI_COUNTRY_POLICY_MANUAL: stream << ":manual"; break;
                    }

                    ESP_LOGI("app/wifi", "%s", stream.str().c_str());

                    delay = 5000 / portTICK_PERIOD_MS;
                }
            }
            catch(::std::exception const &e) { ESP_LOGW("app/wifi", "scan exception caught: %s", e.what()); }
            catch(...) { ESP_LOGW("app/wifi", "unknown scan exception caught"); }
            ::vTaskDelay(delay);
        }
    }

} // namespace tasks

namespace global {

    struct Object final {
        Settings settings;

        inline static auto & instance() noexcept(true) { static Object instance; return instance; }

    private:
        Object() = default;
        template <class T> Object & operator = (T &&) = delete;
        template <class ... T> explicit Object(T && ...) = delete;
    };

} // namespace global

    using Global = global::Object;

namespace settings {

    Object const & get() noexcept(true) { return Global::instance().settings; }

    void set(Object &&settings) noexcept(true) {
        if (settings.temporaryAccessPointSsid.size() > common::wifi::MAX_SSID_LENGTH) settings.temporaryAccessPointSsid.resize(common::wifi::MAX_SSID_LENGTH);
        if (settings.station.ssid.size() > common::wifi::MAX_SSID_LENGTH) settings.station.ssid.resize(common::wifi::MAX_SSID_LENGTH);
        if (settings.station.password.size() > common::wifi::MAX_PASSWORD_LENGTH) settings.station.password.resize(common::wifi::MAX_PASSWORD_LENGTH);
        Global::instance().settings = ::std::move(settings);
        ESP_LOGI("app/wifi", "settings = %s", Global::instance().settings.temporaryAccessPointSsid.c_str());
    }

    void set(Object const &settings) noexcept(true) { return set(::std::decay_t<decltype(settings)>{settings}); }

} // namespace settings

    void init() noexcept(true) {
        static auto flag = false;
        if (flag) return;
        flag = true;

        ESP_LOGI("app/wifi", "initialization started");

        tcpip_adapter_init();
        ESP_LOGI("app/wifi", "tcp/ip adapter initialized");
        ESP_ERROR_CHECK(::esp_event_loop_init(helpers::eventHandler, nullptr));
        ESP_LOGI("app/wifi", "event loop initialized");

        ESP_ERROR_CHECK(::esp_wifi_init(helpers::getInitConfigPointer()));
        ESP_LOGI("app/wifi", "initial config applied");

        ESP_ERROR_CHECK(::esp_wifi_set_mode(::WIFI_MODE_STA));
        ESP_LOGI("app/wifi", "station mode enabled");

//        ::wifi_config_t config;
//        config.ap.ssid[0] = 0;
//        config.ap.password[0] = 0;
//        config.ap.ssid_len = 0;
//        config.ap.channel = 0;
//        config.ap.authmode = ::WIFI_AUTH_OPEN;
//        config.ap.max_connection = 4;
//        config.ap.beacon_interval = 100;
//        config.sta.ssid[0] = 0;
//        config.sta.password[0] = 0;
//        config.sta.scan_method = ::WIFI_ALL_CHANNEL_SCAN;
//        config.sta.bssid_set = 0;
//        config.sta.bssid[0] = 0;
//        config.sta.channel = 0;
//        config.sta.listen_interval = 0;
//        config.sta.sort_method = ::WIFI_CONNECT_AP_BY_SIGNAL;
//        config.sta.threshold.rssi = ::std::numeric_limits<::std::decay_t<decltype(config.sta.threshold.rssi)>>::min();
//        config.sta.threshold.authmode = ::WIFI_AUTH_OPEN;

//        ESP_ERROR_CHECK(::esp_wifi_set_config(::ESP_IF_WIFI_STA, &config));
//        ESP_LOGI("app/wifi", "station interface configured");

        ESP_ERROR_CHECK(::esp_wifi_start());
        ESP_LOGI("app/wifi", "machine started");

        auto status = ::xTaskCreate(tasks::scanner, "app/wifi/scanner", ::std::max(configMINIMAL_STACK_SIZE, 8192), nullptr, 0, nullptr);
        ESP_ERROR_CHECK(pdTRUE == status ? ESP_OK : ESP_FAIL);
        ESP_LOGI("app/wifi", "app/wifi/scanner task started");

        ESP_LOGI("app/wifi", "initialization finished");
    }

} // namespace wifi
} // namespace application
