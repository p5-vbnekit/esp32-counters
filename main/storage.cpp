#include <vector>
#include <stdexcept>
#include <exception>

#include <esp_err.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>

#include "storage.hpp"
#include "common.hpp"


namespace application {
namespace storage {
namespace global {

    struct Object final {
        Data data;
        ::nvs_handle handle = 0;

        inline static auto & instance() noexcept(true) { static Object instance; return instance; }

    private:
        Object() = default;
        template <class T> Object & operator = (T &&) = delete;
        template <class ... T> explicit Object(T && ...) = delete;
    };

} // namespace global

    using Global = global::Object;

namespace data {

    Object const & get() noexcept(true) { return Global::instance().data; }

    void set(Object &&data) noexcept(true) {
        auto changed = false;
        auto &global = Global::instance();

        if (data.counter != global.data.counter) {
            changed = true;
            ESP_ERROR_CHECK(::nvs_set_u32(global.handle, "counter", global.data.counter = data.counter));
            ESP_LOGD("app/storage", "couner value updated");
        }

        if (data.temporaryAccessPointSsid.size() > common::wifi::MAX_SSID_LENGTH) data.temporaryAccessPointSsid.resize(common::wifi::MAX_SSID_LENGTH);
        if (data.temporaryAccessPointSsid != global.data.temporaryAccessPointSsid) {
            changed = true;
            auto const &value = global.data.temporaryAccessPointSsid = ::std::move(data.temporaryAccessPointSsid);
            ESP_ERROR_CHECK(::nvs_set_str(global.handle, "tap_ssid", value.c_str()));
            ESP_LOGD("app/storage", "temporary access point ssid value updated");
        }

        if (changed) {
            ESP_ERROR_CHECK(::nvs_commit(global.handle));
            ESP_LOGD("app/storage", "commit success");
        }
    }

    void set(Object const &data) noexcept(true) { return set(::std::decay_t<decltype(data)>{data}); }

} // namespace data

    void init() noexcept(true) {
        static auto flag = false;
        if (flag) return;
        flag = true;

        auto &global = Global::instance();

        ESP_LOGI("app/storage", "initialization started");

        auto error = ::nvs_flash_init();
        if (error == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            ESP_LOGI("app/storage", "new nvs version of found, erasing");
            ESP_ERROR_CHECK(::nvs_flash_erase());
            error = ::nvs_flash_init();
        }
        ESP_ERROR_CHECK(error);
        ESP_LOGI("app/storage", "nvs initialized");

        ESP_ERROR_CHECK(::nvs_open("application", ::NVS_READWRITE, &global.handle));
        ESP_LOGI("app/storage", "nvs partition opened");

        nvs_stats_t stats;
        ESP_ERROR_CHECK(::nvs_get_stats(nullptr, &stats));
        ESP_LOGI("app/storage", "namespaces = %d, entries: used = %d, free = %d, total = %d",
                 stats.namespace_count, stats.used_entries, stats.free_entries, stats.total_entries);

        ::std::size_t used = 0;
        ESP_ERROR_CHECK(::nvs_get_used_entry_count(global.handle, &used));
        ESP_LOGI("app/storage", "used entry count = %d", used);

        error = ::nvs_get_u32(global.handle, "counter", &global.data.counter);
        if (error == ESP_ERR_NVS_NOT_FOUND) ESP_LOGI("app/storage", "counter value not found");
        else ESP_ERROR_CHECK(error);

        try {
            global.data.temporaryAccessPointSsid = [&global] () -> ::std::decay_t<decltype(global.data.temporaryAccessPointSsid)> {
                ::std::vector<char> buffer(4000, 0);
                ::std::size_t size = buffer.size();

                auto const error = ::nvs_get_str(global.handle, "tap_ssid", buffer.data(), &size);
                if (error == ESP_ERR_NVS_NOT_FOUND) {
                    ESP_LOGI("app/storage", "temporary access point ssid value not found");
                    return {};
                }

                ESP_ERROR_CHECK(error);

                if (! (size < common::wifi::MAX_SSID_LENGTH)) return {buffer.data(), common::wifi::MAX_SSID_LENGTH};
                return buffer.data();
            } ();
        }

        catch(::std::exception const &e) { ESP_LOGW("app/storage", "temporary access point ssid value load exception caught: %s", e.what()); }
        catch(...) { ESP_LOGW("app/storage", "unknown exception of temporary access point ssid value load caught"); }

        ESP_LOGI("app/wifi", "initialization finished");
    }

} // namespace storage
} // namespace application
