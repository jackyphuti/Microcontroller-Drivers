#pragma once

#include <cstdint>

namespace Drivers {

class Gpio {
public:
    enum class Port : std::uintptr_t {
        A = 0x40020000UL,
        B = 0x40020400UL,
        C = 0x40020800UL
    };

    enum class Mode : std::uint32_t {
        Input     = 0b00,
        Output    = 0b01,
        Alternate = 0b10,
        Analog    = 0b11
    };

    enum class OutputType : std::uint32_t {
        PushPull  = 0b0,
        OpenDrain = 0b1
    };

    enum class Speed : std::uint32_t {
        Low      = 0b00,
        Medium   = 0b01,
        Fast     = 0b10,
        High     = 0b11
    };

    enum class Pull : std::uint32_t {
        None     = 0b00,
        PullUp   = 0b01,
        PullDown = 0b10
    };

    Gpio(Port port, std::uint8_t pin);

    void Configure(Mode mode, OutputType otype = OutputType::PushPull, 
                   Speed speed = Speed::Low, Pull pull = Pull::None) const;
    void SetAlternateFunction(std::uint8_t af) const;
    void Write(bool high) const;
    void Toggle() const;
    [[nodiscard]] bool Read() const;

private:
    struct Registers {
        volatile std::uint32_t MODER;
        volatile std::uint32_t OTYPER;
        volatile std::uint32_t OSPEEDR;
        volatile std::uint32_t PUPDR;
        volatile std::uint32_t IDR;
        volatile std::uint32_t ODR;
        volatile std::uint32_t BSRR;
        volatile std::uint32_t LCKR;
        volatile std::uint32_t AFRL;
        volatile std::uint32_t AFRH;
    };

    Registers* const regs_;
    const std::uint8_t pin_;

    static void EnableClock(Port port);
};

} // namespace Drivers