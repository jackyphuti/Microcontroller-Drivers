#include "drivers/gpio.hpp"

namespace {

constexpr std::uintptr_t RCC_AHB1ENR = 0x40023830UL;
constexpr std::uint32_t GPIOA_CLOCK = 1U << 0;

volatile std::uint32_t* const rcc_ahb1enr =
	reinterpret_cast<volatile std::uint32_t*>(RCC_AHB1ENR);

} // namespace

namespace Drivers {

Gpio::Gpio(Port port, std::uint8_t pin)
	: regs_(reinterpret_cast<Registers*>(static_cast<std::uintptr_t>(port))),
	  pin_(pin) {
	EnableClock(port);
}

void Gpio::EnableClock(Port port) {
	if (port == Port::A) {
		*rcc_ahb1enr |= GPIOA_CLOCK;
	}
}

void Gpio::Configure(Mode mode, OutputType otype, Speed speed, Pull pull) const {
	const std::uint32_t shift = static_cast<std::uint32_t>(pin_) * 2U;
	const std::uint32_t mask = 0b11U << shift;

	regs_->MODER = (regs_->MODER & ~mask) |
				   (static_cast<std::uint32_t>(mode) << shift);
	regs_->OSPEEDR = (regs_->OSPEEDR & ~mask) |
					 (static_cast<std::uint32_t>(speed) << shift);
	regs_->PUPDR = (regs_->PUPDR & ~mask) |
				   (static_cast<std::uint32_t>(pull) << shift);

	const std::uint32_t type_mask = 1U << pin_;
	regs_->OTYPER = (regs_->OTYPER & ~type_mask) |
					(static_cast<std::uint32_t>(otype) << pin_);
}

void Gpio::SetAlternateFunction(std::uint8_t af) const {
	const std::uint32_t index = pin_ / 8U;
	const std::uint32_t shift = (static_cast<std::uint32_t>(pin_) % 8U) * 4U;
	volatile std::uint32_t* afr = index == 0U ? &regs_->AFRL : &regs_->AFRH;
	const std::uint32_t mask = 0xFU << shift;
	*afr = (*afr & ~mask) | ((static_cast<std::uint32_t>(af) & 0xFU) << shift);
}

void Gpio::Write(bool high) const {
	regs_->BSRR = high ? (1U << pin_) : (1U << (pin_ + 16U));
}

void Gpio::Toggle() const {
	const std::uint32_t mask = 1U << pin_;
	if ((regs_->ODR & mask) != 0U) {
		Write(false);
	} else {
		Write(true);
	}
}

bool Gpio::Read() const {
	return (regs_->IDR & (1U << pin_)) != 0U;
}

} // namespace Drivers
