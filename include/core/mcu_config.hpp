#pragma once

#include <cstdint>

namespace Core {
namespace Config {

    // Target Selection: STM32F411RE
    inline constexpr std::uint32_t FLASH_SIZE_KB = 512;
    inline constexpr std::uint32_t SRAM_SIZE_KB  = 128;
    
    // Memory Boundaries
    inline constexpr std::uintptr_t FLASH_BASE = 0x08000000UL;
    inline constexpr std::uintptr_t SRAM_BASE  = 0x20000000UL;
    inline constexpr std::uintptr_t SRAM_END   = SRAM_BASE + (SRAM_SIZE_KB * 1024);

    // Clock Configuration (Assuming 16 MHz HSI default)
    inline constexpr std::uint32_t SYSTEM_CORE_CLOCK_HZ = 16'000'000;
    inline constexpr std::uint32_t SYSTICK_PERIOD_MS    = 1;
    inline constexpr std::uint32_t SYSTICK_RELOAD_VAL   = (SYSTEM_CORE_CLOCK_HZ / 1000) * SYSTICK_PERIOD_MS;

    // Vector Table Definition
    inline constexpr std::uint32_t VECTOR_TABLE_OFFSET  = 0x00000000UL;

} // namespace Config
} // namespace Core