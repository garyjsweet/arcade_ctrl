cmake_minimum_required(VERSION 3.13)

# Initialize pico-sdk from submodule
# note: this must happen before project()
include(pico-sdk/pico_sdk_init.cmake)

project(ArcadeCtrl)

set(PICO_SDK_PATH ${CMAKE_CURRENT_LIST_DIR}/pico-sdk)

# Initialize the SDK
pico_sdk_init()

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PUBLIC
        main.cpp
        Encoder.cpp
        Analog.cpp
        ArcadeCtrl.cpp
        USB.cpp
        BlinkLED.cpp
        Encoder.pio
        )

# Make sure TinyUSB can find tusb_config.h
target_include_directories(${PROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_LIST_DIR})

target_include_directories(${PROJECT_NAME} PRIVATE
        encoder/src)

add_custom_target(EncoderPioHeader
                  ${CMAKE_CURRENT_BINARY_DIR}/pioasm/pioasm
                  ${CMAKE_CURRENT_LIST_DIR}/Encoder.pio
                  ${CMAKE_CURRENT_LIST_DIR}/EncoderPio.h)

add_dependencies(EncoderPioHeader PioasmBuild)

add_dependencies(${PROJECT_NAME} EncoderPioHeader)

# In addition to pico_stdlib required for common PicoSDK functionality, add dependency on tinyusb_device
# for TinyUSB device support and tinyusb_board for the additional board support library used by the example
target_link_libraries(${PROJECT_NAME} PUBLIC pico_stdlib tinyusb_device tinyusb_board hardware_gpio hardware_adc hardware_pio)

# Uncomment this line to enable fix for Errata RP2040-E5 (the fix requires use of GPIO 15)
#target_compile_definitions(${PROJECT_NAME} PUBLIC PICO_RP2040_USB_DEVICE_ENUMERATION_FIX=1)

pico_add_extra_outputs(${PROJECT_NAME})
