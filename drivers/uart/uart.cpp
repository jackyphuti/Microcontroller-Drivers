#include "drivers/uart.hpp"
#include "drivers/gpio.hpp"
#include "core/system.hpp"

namespace {

constexpr std::uintptr_t RCC_AHB1ENR = 0x40023830UL;
constexpr std::uintptr_t RCC_APB1ENR = 0x40023840UL;
constexpr std::uint32_t GPIOA_CLOCK = 1U << 0;
constexpr std::uint32_t USART2_CLOCK = 1U << 17;

volatile std::uint32_t* const rcc_ahb1enr =
	reinterpret_cast<volatile std::uint32_t*>(RCC_AHB1ENR);
volatile std::uint32_t* const rcc_apb1enr =
	reinterpret_cast<volatile std::uint32_t*>(RCC_APB1ENR);

} // namespace

namespace Drivers {

Core::RingBuffer<char, 256> g_rx_buffer;

Uart::Uart(Instance instance, std::uint32_t baudrate)
	: regs_(reinterpret_cast<Registers*>(static_cast<std::uintptr_t>(instance))) {
	EnableClock(instance);

	// USART2 uses PA2 (TX) and PA3 (RX) with alternate-function mapping 7.
	Gpio tx(Gpio::Port::A, 2);
	Gpio rx(Gpio::Port::A, 3);
	tx.Configure(Gpio::Mode::Alternate, Gpio::OutputType::PushPull, Gpio::Speed::High, Gpio::Pull::None);
	rx.Configure(Gpio::Mode::Alternate, Gpio::OutputType::PushPull, Gpio::Speed::High, Gpio::Pull::PullUp);
	tx.SetAlternateFunction(7);
	rx.SetAlternateFunction(7);

	regs_->BRR = (16000000UL + (baudrate / 2U)) / baudrate;
	regs_->CR1 = (1U << 13) | (1U << 3) | (1U << 2);
}

void Uart::EnableClock(Instance instance) {
	if (instance == Instance::Uart2) {
		*rcc_ahb1enr |= GPIOA_CLOCK;
		*rcc_apb1enr |= USART2_CLOCK;
	}
}

bool Uart::Write(char c, std::uint32_t timeout_ms) const {
	const std::uint64_t start_time = Core::GetTick();
    while ((regs_->SR & (1U << 7)) == 0U) {
		if ((Core::GetTick() - start_time) >= timeout_ms) {
			return false;
		}
	}
	regs_->DR = static_cast<std::uint32_t>(static_cast<unsigned char>(c));
	return true;
}

bool Uart::Write(const char* message, std::uint32_t timeout_ms) const {
	while (*message != '\0') {
		if (!Write(*message++, timeout_ms)) {
			return false;
		}
	}
	return true;
}

bool Uart::Write(std::string_view message, std::uint32_t timeout_ms) const {
	for (const char c : message) {
		if (!Write(c, timeout_ms)) {
			return false;
		}
	}
	return true;
}

std::optional<char> Uart::Read(std::uint32_t timeout_ms) const {
	const std::uint64_t start_time = Core::GetTick();
	while (!HasData()) {
		if ((Core::GetTick() - start_time) >= timeout_ms) {
			return std::nullopt;
		}
	}
	return static_cast<char>(regs_->DR & 0xFFU);
}

bool Uart::HasData() const {
	return (regs_->SR & (1U << 5)) != 0U;
}

void Uart::EnableInterrupts() const {
	regs_->CR1 |= (1U << 5);
	auto* const nvic_iser1 = reinterpret_cast<volatile std::uint32_t*>(0xE000E104UL);
	*nvic_iser1 |= (1U << 6);
}

} // namespace Drivers

extern "C" void USART2_IRQHandler() {
	auto* const status = reinterpret_cast<volatile std::uint32_t*>(0x40004400UL);
	auto* const data = reinterpret_cast<volatile std::uint32_t*>(0x40004404UL);
	const std::uint32_t sr = *status;
	if ((sr & (1U << 5)) != 0U) { // RXNE
		Drivers::g_rx_buffer.Push(static_cast<char>(*data & 0xFFU));
	} else if ((sr & (1U << 3)) != 0U) { // ORE: Overrun Error
		(void)*data; // Reading DR clears ORE after SR has been read
	}
}
