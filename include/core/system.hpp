#pragma once

#include <cstdint>

namespace Core {

// Core Peripheral Base Addresses
inline constexpr std::uintptr_t SCS_BASE   = 0xE000E000UL;
inline constexpr std::uintptr_t SYSTICK_BASE = SCS_BASE + 0x0010UL;

struct SysTick_Type {
    volatile std::uint32_t CTRL;
    volatile std::uint32_t LOAD;
    volatile std::uint32_t VAL;
    volatile std::uint32_t CALIB;
};

inline auto* const SysTick = reinterpret_cast<SysTick_Type*>(SYSTICK_BASE);

void SystemInit();
void DelayMs(std::uint32_t ms);
void IncrementTick();
std::uint64_t GetTick();

} // namespace Core