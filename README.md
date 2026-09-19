# Bare-Metal ARM Cortex-M4 Embedded Driver Framework

A freestanding, low-level C++20 driver implementation and firmware foundation targeting ARM Cortex-M4 microcontrollers (STM32F4 architecture). 

This firmware runs directly on the bare metal without any vendor hardware abstraction layers (HALs), standard C runtime libraries, or Real-Time Operating Systems (RTOS).

## Architecture Highlights

- **C++20 Zero-Cost Abstractions**: Modern strongly typed peripheral register mapped structures (`constexpr` addresses, namespaces, class-encapsulated peripheral blocks).
- **Custom Vector Table & Runtime**: Custom `startup.cpp` featuring direct `.data` copy from Flash ROM to SRAM, zero-initialization of `.bss`, and dynamic execution of C++ global constructors (`__init_array`).
- **SysTick Preemption Counter**: Hardware-driven millisecond timing using ARM Core SysTick interrupts paired with low-power state idling (`wfi`).
- **Memory-Mapped Drivers**: Direct hardware register implementations for GPIO pin driving and high-speed USART2 full-duplex transmission.

## Hardware Targets

- **Core**: ARM Cortex-M4 (Thumb-2 ISA with Hard-Float VFPv4-SP)
- **Base Clock**: 16 MHz HSI
- **Peripherals**: GPIOA (Pin 5 Status LED), USART2 (PA2 TX / PA3 RX at 115200 Baud)
- **Tested Hardware**: STMicroelectronics STM32F401RE / STM32F411RE Nucleo boards.

## Toolchain Prerequisites

### Fedora Linux (WSL or Native)
