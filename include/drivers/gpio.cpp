#include "drivers/gpio.hpp"

namespace Drivers {

inline constexpr std::uintptr_t RCC_AHB1ENR = 0x40023830UL;

void Gpio::EnableClock(Port port) {
    auto* ahb1 = reinterpret_cast<volatile std::uint32_t*>(RCC_AHB1ENR);
    switch (port) {
        case Port::A: *ahb1 |= (1U << 0); break;
        case Port::B: *ahb1 |= (1U << 1); break;
        case Port::C: *ahb1 |= (1U << 2); break;
    }
}

Gpio::Gpio(Port port, std::uint8_t pin) 
    : regs_(reinterpret_cast<Registers*>(static_cast<std::uintptr_t>(port))), pin_(pin) {
    EnableClock(port);
}

void Gpio::Configure(Mode mode, OutputType otype, Speed speed, Pull pull) const {
    const auto pin_pos = static_cast<std::uint32_t>(pin_);

    // Mode
    regs_->MODER &= ~(0x3U << (pin_pos * 2U));
    regs_->MODER |= (static_cast<std::uint32_t>(mode) << (pin_pos * 2U));

    // Output Type
    regs_->OTYPER &= ~(0x1U << pin_pos);
    regs_->OTYPER |= (static_cast<std::uint32_t>(otype) << pin_pos);

    // Speed
    regs_->OSPEEDR &= ~(0x3U << (pin_pos * 2U));
    regs_->OSPEEDR |= (static_cast<std::uint32_t>(speed) << (pin_pos * 2U));

    // Pull-up/down
    regs_->PUPDR &= ~(0x3U << (pin_pos * 2U));
    regs_->PUPDR |= (static_cast<std::uint32_t>(pull) << (pin_pos * 2U));
}

void Gpio::SetAlternateFunction(std::uint8_t af) const {
    const auto pin_pos = static_cast<std::uint32_t>(pin_);
    if (pin_ < 8) {
        regs_->AFRL &= ~(0xFU << (pin_pos * 4U));
        regs_->AFRL |= (static_cast<std::uint32_t>(af & 0xFU) << (pin_pos * 4U));
    } else {
        const auto high_pin = pin_pos - 8U;
        regs_->AFRH &= ~(0xFU << (high_pin * 4U));
        regs_->AFRH |= (static_cast<std::uint32_t>(af & 0xFU) << (high_pin * 4U));
    }
}

void Gpio::Write(bool high) const {
    if (high) {
        regs_->BSRR = (1U << pin_); // Set pin
    } else {
        regs_->BSRR = (1U << (pin_ + 16U)); // Reset pin
    }
}

void Gpio::Toggle() const {
    regs_->ODR ^= (1U << pin_);
}

bool Gpio::Read() const {
    return (regs_->IDR & (1U << pin_)) != 0;
}

} // namespace Drivers