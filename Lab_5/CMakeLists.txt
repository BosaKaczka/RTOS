# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(Lab_5 C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()
include(FreeRTOS_Kernel_import.cmake)

# Header files live in inc/
include_directories(inc)

# Add executable with all source files
add_executable(Lab_5
    Lab_5.c
    src/ssd1306.c
    src/lis3dh.c
)

pico_set_program_name(Lab_5 "Lab_5")
pico_set_program_version(Lab_5 "0.1")

# Output over both UART and USB
pico_enable_stdio_uart(Lab_5 1)
pico_enable_stdio_usb(Lab_5 1)

# Include directories
target_include_directories(Lab_5 PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/inc
)

# Link libraries
target_link_libraries(Lab_5
    pico_stdlib
    FreeRTOS-Kernel-Heap4
    pico_cyw43_arch_none
    hardware_i2c
)

pico_add_extra_outputs(Lab_5)
