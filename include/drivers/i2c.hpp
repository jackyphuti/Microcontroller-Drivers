#pragma once

#include <cstdint>
#include <span>

namespace Drivers {

class I2c {
public:
    enum class Instance : std::uintptr_t {
        I2c1 = 0x40005400UL // APB1 Peripheral
    };

    explicit I2c(Instance instance);

    // I2C Hardware interactions are highly state-dependent. 
    // Returns true on success, false if hardware NACKs or times out.
    bool Write(std::uint8_t device_address, std::span<const std::uint8_t> data) const;
    bool Read(std::uint8_t device_address, std::span<std::uint8_t> buffer) const;

private:
    struct Registers {
        volatile std::uint32_t CR1;
        volatile std::uint32_t CR2;
        volatile std::uint32_t OAR1;
        volatile std::uint32_t OAR2;
        volatile std::uint32_t DR;
        volatile std::uint32_t SR1;
        volatile std::uint32_t SR2;
        volatile std::uint32_t CCR;
        volatile std::uint32_t TRISE;
        volatile std::uint32_t FLTR;
    };

    Registers* const regs_;
    static void EnableClock(Instance instance);
};

} // namespace Drivers