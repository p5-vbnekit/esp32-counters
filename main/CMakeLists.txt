set(COMPONENT_SRCS
    "main.cpp"
    "chip_info.cpp" "chip_info.hpp"
    "gpio.cpp" "gpio.hpp"
    "counter.cpp" "counter.hpp"
    "wifi.cpp" "wifi.hpp"
    "storage.cpp" "storage.hpp"
    "common.hpp"
)

set(COMPONENT_ADD_INCLUDEDIRS "")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

register_component()
