#include <esp_system.h>
#include <esp_spi_flash.h>
#include <esp_log.h>

#include "chip_info.hpp"


namespace application {
namespace chip_info {

    void execute() noexcept(true) {
        ::esp_chip_info_t chip_info;
        ::esp_chip_info(&chip_info);

        ESP_LOGI("app/chip_info", "This is ESP32 chip with %d CPU cores, WiFi%s%s, ",
                 chip_info.cores,
                 (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
                 (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

        ESP_LOGI("app/chip_info", "silicon revision %d, ", chip_info.revision);

        ESP_LOGI("app/chip_info", "%fMB %s flash", spi_flash_get_chip_size() / (1024.0 * 1024),
                 (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
    }

} // namespace chip_info
} // namespace application
