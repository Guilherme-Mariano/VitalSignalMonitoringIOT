# Versão mínima do CMake necessária
cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(WifiTcp C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Configurações específicas para o lwIP e CYW43
set(PICO_CYW43_ARCH_THREADSAFE_BACKGROUND ON)  # Habilita o modo thread-safe para o CYW43
set(PICO_LWIP_MEM_LIBC_MALLOC OFF)             # Desativa o uso de malloc do libc no lwIP

# Add executable. Default name is the project name, version 0.1
add_executable(WifiTcp WifiTcp.c)

pico_set_program_name(WifiTcp "WifiTcp")
pico_set_program_version(WifiTcp "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(WifiTcp 0)  # Desabilita stdio via UART
pico_enable_stdio_usb(WifiTcp 1)   # Habilita stdio via USB (útil para depuração)

# Add the standard library to the build
target_link_libraries(WifiTcp
    pico_stdlib
    hardware_watchdog
    hardware_irq
    hardware_pwm
    hardware_dma
    hardware_spi
    hardware_i2c
    hardware_adc
    hardware_uart
    pico_cyw43_arch_lwip_threadsafe_background
    pico_lwip
)

# Add the standard include files to the build
target_include_directories(WifiTcp PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(WifiTcp 
    pico_cyw43_arch_lwip_threadsafe_background
)

pico_add_extra_outputs(WifiTcp)