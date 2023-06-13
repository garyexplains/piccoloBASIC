cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_PATH "/home/gary/pico/pico-sdk")

set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

project(piccoloBASIC C CXX ASM)

# Initialize the SDK
pico_sdk_init()

if (TARGET tinyusb_device)
    add_executable(piccoloBASIC piccoloBASIC.c piccoloBASIC.h tokenizer.c tokenizer.h ubasic.c ubasic.h lfs.c lfs.h lfs_util.c lfs_util.h lfs_wrapper.c)

    # pull in common dependencies
    target_link_libraries(piccoloBASIC pico_stdlib hardware_flash)

    # enable usb output, disable uart output
    pico_enable_stdio_usb(piccoloBASIC 1)
    pico_enable_stdio_uart(piccoloBASIC 0)

    # Add the standard include files to the build
    target_include_directories(piccoloBASIC PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts or any other standard includes, if required
    )

    # create map/bin/hex/uf2 file etc.
    pico_add_extra_outputs(piccoloBASIC)

elseif(PICO_ON_DEVICE)
    message(WARNING "not building hello_usb because TinyUSB submodule is not initialized in the SDK")
endif()
