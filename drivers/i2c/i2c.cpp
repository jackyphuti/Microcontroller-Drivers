#include "drivers/i2c.hpp"
#include "drivers/gpio.hpp"

namespace Drivers {

namespace {

constexpr std::uintptr_t RCC_APB1ENR = 0x40023840UL;

bool WaitForFlag(volatile std::uint32_t& reg, std::uint32_t mask, bool set, std::uint32_t timeout_cycles = 100000) {
    while (((reg & mask) != 0U) != set) {
        if (--timeout_cycles == 0U) {
            return false;
        }
    }
    return true;
}

} // namespace

void I2c::EnableClock(Instance instance) {
    if (instance == Instance::I2c1) {
        auto* apb1 = reinterpret_cast<volatile std::uint32_t*>(RCC_APB1ENR);
        *apb1 |= (1U << 21); // Enable I2C1 clock
    }
}

I2c::I2c(Instance instance) 
    : regs_(reinterpret_cast<Registers*>(static_cast<std::uintptr_t>(instance))) {
    
    // Configure GPIO Pins for I2C1: PB6 (SCL), PB7 (SDA)
    // I2C strictly requires Open-Drain output type to allow external pull-up resistors to pull the line high.
    Gpio scl(Gpio::Port::B, 6);
    Gpio sda(Gpio::Port::B, 7);
    
    scl.Configure(Gpio::Mode::Alternate, Gpio::OutputType::OpenDrain, Gpio::Speed::High, Gpio::Pull::PullUp);
    sda.Configure(Gpio::Mode::Alternate, Gpio::OutputType::OpenDrain, Gpio::Speed::High, Gpio::Pull::PullUp);
    
    scl.SetAlternateFunction(4);
    sda.SetAlternateFunction(4);

    EnableClock(instance);

    // Software Reset the I2C Block
    regs_->CR1 |= (1U << 15);
    regs_->CR1 &= ~(1U << 15);

    // Set Peripheral Clock Frequency (16 MHz APB1)
    regs_->CR2 = 16;

    // Configure Clock Control Register (CCR) for Standard Mode (100 kHz)
    // T_high = CCR * T_PCLK1 = 80 * (1 / 16MHz) = 5 us. (Period = 10 us -> 100 kHz)
    regs_->CCR = 80;

    // Maximum rise time in Standard Mode is 1000ns. 
    // TRISE = (1000ns / 62.5ns) + 1 = 17
    regs_->TRISE = 17;

    // Enable I2C Peripheral
    regs_->CR1 |= (1U << 0); // PE
}

bool I2c::Write(std::uint8_t device_address, std::span<const std::uint8_t> data) const {
    // Wait for any previous STOP generation to complete
    if (!WaitForFlag(regs_->CR1, (1U << 9), false)) {
        return false;
    }

    // Wait until the I2C bus is not busy
    if (!WaitForFlag(regs_->SR2, (1U << 1), false)) {
        // Reset peripheral if bus is locked
        regs_->CR1 |= (1U << 15);
        regs_->CR1 &= ~(1U << 15);
        regs_->CR1 |= (1U << 0);
        return false;
    }

    // Generate START condition
    regs_->CR1 |= (1U << 8); 
    if (!WaitForFlag(regs_->SR1, (1U << 0), true)) { // Wait for SB
        regs_->CR1 |= (1U << 9);
        return false;
    }

    // Send Slave Address with Write bit (0)
    regs_->DR = static_cast<std::uint32_t>(device_address << 1);
    
    // Wait for ADDR flag or AF (Acknowledge Failure)
    std::uint32_t timeout = 50000;
    while (!(regs_->SR1 & (1U << 1))) {
        if (regs_->SR1 & (1U << 10)) { // AF
            regs_->CR1 |= (1U << 9);   // Generate STOP
            regs_->SR1 &= ~(1U << 10); // Clear AF
            WaitForFlag(regs_->CR1, (1U << 9), false);
            return false;
        }
        if (--timeout == 0U) {
            regs_->CR1 |= (1U << 9);
            return false;
        }
    }
    
    // Clear ADDR flag by reading SR1 followed by SR2
    (void)regs_->SR1;
    (void)regs_->SR2;

    if (data.empty()) {
        // 0-byte write probe: address acknowledged, generate STOP and return success
        regs_->CR1 |= (1U << 9);
        WaitForFlag(regs_->CR1, (1U << 9), false);
        return true;
    }

    for (std::uint8_t byte : data) {
        if (!WaitForFlag(regs_->SR1, (1U << 7), true)) { // Wait for TXE
            regs_->CR1 |= (1U << 9);
            return false;
        }
        regs_->DR = byte;
    }

    if (!WaitForFlag(regs_->SR1, (1U << 2), true)) { // Wait for BTF
        regs_->CR1 |= (1U << 9);
        return false;
    }
    
    // Generate STOP condition
    regs_->CR1 |= (1U << 9); 
    WaitForFlag(regs_->CR1, (1U << 9), false);
    return true;
}

bool I2c::Read(std::uint8_t device_address, std::span<std::uint8_t> buffer) const {
    if (buffer.empty()) {
        return true;
    }

    // Wait for any previous STOP generation to complete
    if (!WaitForFlag(regs_->CR1, (1U << 9), false)) {
        return false;
    }

    // Wait until the bus is not busy
    if (!WaitForFlag(regs_->SR2, (1U << 1), false)) {
        regs_->CR1 |= (1U << 15);
        regs_->CR1 &= ~(1U << 15);
        regs_->CR1 |= (1U << 0);
        return false;
    }

    if (buffer.size() == 1) {
        regs_->CR1 &= ~(1U << 10); // Clear ACK for 1-byte reception
    } else {
        regs_->CR1 |= (1U << 10);  // Enable ACK for multi-byte reception
    }

    // Generate START
    regs_->CR1 |= (1U << 8); 
    if (!WaitForFlag(regs_->SR1, (1U << 0), true)) { // Wait for SB
        regs_->CR1 |= (1U << 9);
        return false;
    }

    // Send Slave Address with Read bit (1)
    regs_->DR = static_cast<std::uint32_t>((device_address << 1) | 1);
    
    // Wait for ADDR flag or AF
    std::uint32_t timeout = 50000;
    while (!(regs_->SR1 & (1U << 1))) {
        if (regs_->SR1 & (1U << 10)) { // AF
            regs_->CR1 |= (1U << 9);   // Generate STOP
            regs_->SR1 &= ~(1U << 10); // Clear AF
            WaitForFlag(regs_->CR1, (1U << 9), false);
            return false;
        }
        if (--timeout == 0U) {
            regs_->CR1 |= (1U << 9);
            return false;
        }
    }

    // 1-byte read sequence per STM32 hardware specification
    if (buffer.size() == 1) {
        regs_->CR1 &= ~(1U << 10); // Ensure ACK cleared
        (void)regs_->SR1;          // Clear ADDR
        (void)regs_->SR2;
        regs_->CR1 |= (1U << 9);   // Generate STOP before reading data

        if (!WaitForFlag(regs_->SR1, (1U << 6), true)) { // Wait for RXNE
            return false;
        }
        buffer[0] = static_cast<std::uint8_t>(regs_->DR);
        WaitForFlag(regs_->CR1, (1U << 9), false);
        return true;
    }

    // N-byte read sequence
    (void)regs_->SR1; // Clear ADDR
    (void)regs_->SR2;

    for (std::size_t i = 0; i < buffer.size(); ++i) {
        if (i == buffer.size() - 1) {
            regs_->CR1 &= ~(1U << 10); // Clear ACK for the last byte
            regs_->CR1 |= (1U << 9);   // Generate STOP before reading the last byte
        }

        if (!WaitForFlag(regs_->SR1, (1U << 6), true)) { // Wait for RXNE
            return false;
        }
        buffer[i] = static_cast<std::uint8_t>(regs_->DR);
    }

    WaitForFlag(regs_->CR1, (1U << 9), false);
    return true;
}

} // namespace Drivers
