#include "drivers/watchdog.hpp"

namespace Drivers {

// IWDG Base Address for STM32F4 series
Watchdog::Registers* const Watchdog::regs_ = reinterpret_cast<Watchdog::Registers*>(0x40003000UL);

void Watchdog::Enable(std::uint32_t timeout_ms) {
    // Clamp timeout to the 12-bit register maximum (4095 ms)
    if (timeout_ms > 4095) {
        timeout_ms = 4095;
    }

    // 1. Write the magic key to enable access to PR and RLR
    regs_->KR = 0x5555; 

    // 2. Set prescaler to /32. 
    // LSI is ~32 kHz. 32000 / 32 = 1000 Hz (1 tick = 1 ms).
    regs_->PR = 0x03; 

    // 3. Set the countdown reload value
    regs_->RLR = timeout_ms; 

    // 4. Wait for the status register to confirm the updates are written to the LSI domain
    while (regs_->SR != 0) {
        asm volatile("nop");
    }

    // 5. Start the watchdog (This automatically turns on the physical LSI oscillator)
    regs_->KR = 0xCCCC; 

    // 6. Perform the first reload to initialize the counter
    ResetTimer();
}

void Watchdog::ResetTimer() {
    // Write the reload key to reset the countdown to the RLR value
    regs_->KR = 0xAAAA; 
}

} // namespace Drivers