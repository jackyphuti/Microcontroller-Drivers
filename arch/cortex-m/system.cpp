#include "core/system.hpp"
#include "core/mcu_config.hpp"

namespace Core {

namespace {
volatile std::uint64_t g_system_ticks = 0;
}

inline constexpr std::uintptr_t RCC_BASE = 0x40023800UL;
inline auto* const RCC_CR   = reinterpret_cast<volatile std::uint32_t*>(RCC_BASE + 0x00UL);
inline auto* const RCC_CFGR = reinterpret_cast<volatile std::uint32_t*>(RCC_BASE + 0x08UL);

inline constexpr std::uintptr_t FLASH_R_BASE = 0x40023C00UL;
inline auto* const FLASH_ACR = reinterpret_cast<volatile std::uint32_t*>(FLASH_R_BASE + 0x00UL);

inline constexpr std::uintptr_t SCB_VTOR = 0xE000ED08UL;

void SystemInit() {
    // 1. Relocate the Vector Table (Crucial if booted from a custom bootloader)
    *reinterpret_cast<volatile std::uint32_t*>(SCB_VTOR) = 
        Config::FLASH_BASE | Config::VECTOR_TABLE_OFFSET;

    // 2. Configure Flash Access Control (Enable Prefetch, Instruction/Data Cache, 0 Wait States for 16MHz)
    *FLASH_ACR = (1U << 8) | (1U << 9) | (1U << 10) | (0U << 0);

    // 3. Enable High-Speed Internal oscillator (16 MHz HSI)
    *RCC_CR |= (1U << 0);
    
    // 4. Verify Clock Synchronization (Deterministic wait with a timeout safeguard)
    std::uint32_t timeout = 10000;
    while (!(*RCC_CR & (1U << 1)) && --timeout) {
        asm volatile("nop");
    }
    if (timeout == 0) {
        // Clock failed to stabilize. Trigger a software reset or trap.
        asm volatile("bkpt #0");
    }

    // 5. Reset clock configuration register (Run directly off HSI 16 MHz)
    *RCC_CFGR &= ~0x00000003U;

    // 6. Start SysTick
    SysTick->LOAD = Config::SYSTICK_RELOAD_VAL - 1UL;
    SysTick->VAL  = 0UL;
    SysTick->CTRL = (1U << 2) | (1U << 1) | (1U << 0);
}

void IncrementTick() {
    g_system_ticks = g_system_ticks + 1;
}

std::uint64_t GetTick() {
    return g_system_ticks;
}

void DelayMs(std::uint32_t ms) {
    const std::uint64_t start = GetTick();
    while ((GetTick() - start) < ms) {
        asm volatile("wfi");
    }
}

}