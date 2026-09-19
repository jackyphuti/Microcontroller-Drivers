#pragma once

#include <cstdint>

namespace Drivers {

class Watchdog {
public:
    // Enables the IWDG. Once enabled, it cannot be turned off by software.
    // The maximum timeout with a /32 prescaler is 4095 milliseconds.
    static void Enable(std::uint32_t timeout_ms);

    // Feeds the watchdog to prevent a system reset
    static void ResetTimer();

private:
    struct Registers {
        volatile std::uint32_t KR;  // Key register
        volatile std::uint32_t PR;  // Prescaler register
        volatile std::uint32_t RLR; // Reload register
        volatile std::uint32_t SR;  // Status register
    };

    static Registers* const regs_;
};

} // namespace Drivers