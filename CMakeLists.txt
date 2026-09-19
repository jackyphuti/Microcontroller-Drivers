cmake_minimum_required(VERSION 3.20)
project(mcu-baremetal-firmware C CXX ASM)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_SYSTEM_NAME STREQUAL "Generic")
    add_executable(${CMAKE_PROJECT_NAME}.elf
        arch/cortex-m/startup.cpp
        arch/cortex-m/system.cpp
        drivers/gpio/gpio.cpp
        drivers/uart/uart.cpp
        include/drivers/watchdog/watchdog.cpp
        include/drivers/i2c/i2c.cpp
        include/drivers/spi/spi.cpp
        src/main.cpp
    )

    target_include_directories(${CMAKE_PROJECT_NAME}.elf PRIVATE include)

    target_compile_options(${CMAKE_PROJECT_NAME}.elf PRIVATE
        -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
        -ffreestanding -fdata-sections -ffunction-sections
        -Wall -Wextra -Wpedantic -Wconversion -Wshadow
        -O2 -g3
        -fno-exceptions -fno-rtti -fno-unwind-tables
    )

    set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/ldscripts/flash.ld)
    target_link_options(${CMAKE_PROJECT_NAME}.elf PRIVATE
        -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
        -T${LINKER_SCRIPT}
        -nostdlib
        -Wl,--gc-sections
        -Wl,-Map=${CMAKE_PROJECT_NAME}.map
        -Wl,--print-memory-usage
    )

    add_custom_command(TARGET ${CMAKE_PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary ${CMAKE_PROJECT_NAME}.elf ${CMAKE_PROJECT_NAME}.bin
        COMMAND ${CMAKE_OBJCOPY} -O ihex ${CMAKE_PROJECT_NAME}.elf ${CMAKE_PROJECT_NAME}.hex
        COMMAND ${CMAKE_SIZE} --format=berkeley ${CMAKE_PROJECT_NAME}.elf
        COMMENT "Generating release artifacts and analyzing memory..."
    )
else()
    enable_testing()

    add_executable(host_tests tests/test_ring_buffer.cpp)
    target_compile_options(host_tests PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME HostLogicTests COMMAND host_tests)
endif()