#include "core/system.hpp"
#include <cstdint>

// Linker-provided symbols
extern "C" {
    extern std::uint32_t _estack;
    extern std::uint32_t _sidata;
    extern std::uint32_t _sdata;
    extern std::uint32_t _edata;
    extern std::uint32_t _sbss;
    extern std::uint32_t _ebss;

    extern void (*__init_array_start)();
    extern void (*__init_array_end)();

    int main();
    [[noreturn]] void Reset_Handler() noexcept;
    [[noreturn]] void Default_Handler() noexcept;
}

// Exception handlers declarations with weak linkage
[[noreturn]] void NMI_Handler() noexcept        __attribute__((weak, alias("Default_Handler")));
extern "C" [[noreturn]] void HardFault_Handler() noexcept;
[[noreturn]] void MemManage_Handler() noexcept  __attribute__((weak, alias("Default_Handler")));
[[noreturn]] void BusFault_Handler() noexcept   __attribute__((weak, alias("Default_Handler")));
[[noreturn]] void UsageFault_Handler() noexcept __attribute__((weak, alias("Default_Handler")));
[[noreturn]] void SVC_Handler() noexcept        __attribute__((weak, alias("Default_Handler")));
[[noreturn]] void DebugMon_Handler() noexcept   __attribute__((weak, alias("Default_Handler")));
[[noreturn]] void PendSV_Handler() noexcept     __attribute__((weak, alias("Default_Handler")));

extern "C" void SysTick_Handler() {
    Core::IncrementTick();
}

// Peripheral Interrupt Handlers
void USART2_IRQHandler()  __attribute__((weak, alias("Default_Handler")));

__attribute__((section(".isr_vector"), used))
const IsrHandler g_vector_table[] = {
    reinterpret_cast<IsrHandler>(&_estack),
    Reset_Handler, NMI_Handler, HardFault_Handler, MemManage_Handler,
    BusFault_Handler, UsageFault_Handler, nullptr, nullptr, nullptr, nullptr,
    SVC_Handler, DebugMon_Handler, nullptr, PendSV_Handler, SysTick_Handler,

    // Peripheral Interrupts (IRQs 0 to 37 padded with nullptrs)
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    
    // IRQ 38: USART2 Global Interrupt
    USART2_IRQHandler 
};

// Vector Table matching Cortex-M hardware specification
using IsrHandler = void (*)();

__attribute__((section(".isr_vector"), used))
const IsrHandler g_vector_table[] = {
    reinterpret_cast<IsrHandler>(&_estack), // Initial Stack Pointer value
    Reset_Handler,
    NMI_Handler,
    HardFault_Handler,
    MemManage_Handler,
    BusFault_Handler,
    UsageFault_Handler,
    nullptr, nullptr, nullptr, nullptr,     // Reserved
    SVC_Handler,
    DebugMon_Handler,
    nullptr,                                // Reserved
    PendSV_Handler,
    SysTick_Handler,
};

[[noreturn]] void Default_Handler() noexcept {
    while (true) {
        asm volatile("bkpt #0");
    }
}

[[noreturn]] void Reset_Handler() noexcept {
    // 1. Copy initialized .data section from Flash to SRAM
    auto* src = &_sidata;
    auto* dst = &_sdata;
    while (dst < &_edata) {
        *dst++ = *src++;
    }

    // 2. Zero-out uninitialized .bss section in SRAM
    auto* bss = &_sbss;
    while (bss < &_ebss) {
        *bss++ = 0U;
    }

    // 3. Execute global constructors
    const auto init_count = &__init_array_end - &__init_array_start;
    for (std::ptrdiff_t i = 0; i < init_count; ++i) {
        (&__init_array_start)[i]();
    }

    // 4. Configure low-level clocks and timers
    Core::SystemInit();

    // 5. Transfer execution to application entry point
    main();

    // Catch infinite loop if main returns
    while (true) {
        asm volatile("wfe");
    }
}

// A struct to map the CPU registers pushed to the stack during a fault
struct FaultFrame {
    std::uint32_t r0, r1, r2, r3, r12, lr, pc, xpsr;
};

// Store fault data in an uninitialized RAM section so it survives a reboot
[[gnu::section(".noinit")]] volatile FaultFrame g_crash_state;

extern "C" void HardFault_Analyze(FaultFrame* frame) {
    // 1. Capture the exact registers at the moment of the crash
    g_crash_state.r0 = frame->r0;
    g_crash_state.pc = frame->pc;
    g_crash_state.lr = frame->lr;

    // 2. Trigger an immediate System Reset via the Cortex-M Application Interrupt and Reset Control Register
    constexpr std::uintptr_t SCB_AIRCR = 0xE000ED0CUL;
    *reinterpret_cast<volatile std::uint32_t*>(SCB_AIRCR) = (0x5FAUL << 16) | (1U << 2);
    
    while(true) { asm volatile("wfi"); } // Wait for reset to take effect
}

// Naked assembly handler to determine which stack pointer (MSP or PSP) was active before the fault
extern "C" __attribute__((naked, noreturn)) void HardFault_Handler() noexcept {
    asm volatile(
        "tst lr, #4 \n"            // Test bit 2 of Link Register
        "ite eq \n"                // If Equal (0), we were using Main Stack Pointer
        "mrseq r0, msp \n"         // Move MSP to R0
        "mrsne r0, psp \n"         // Else, Move Process Stack Pointer to R0
        "b HardFault_Analyze \n"   // Branch to C++ analyzer, passing R0 as the FaultFrame pointer
    );
}