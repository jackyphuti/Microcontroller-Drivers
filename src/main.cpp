#include "core/system.hpp"
#include "drivers/gpio.hpp"
#include "drivers/uart.hpp"
#include "drivers/watchdog.hpp"

int main() {
    Drivers::Gpio led(Drivers::Gpio::Port::A, 5);
    led.Configure(Drivers::Gpio::Mode::Output, Drivers::Gpio::OutputType::PushPull);

    Drivers::Uart console(Drivers::Uart::Instance::Uart2, 115200);

    // Enable the watchdog with a 2000 ms (2 second) timeout
    Drivers::Watchdog::Enable(2000);

    console.Write("\r\n[BOOT] System started. Watchdog armed (2000ms).\r\n");

    while (true) {
        // "Pet" the dog at the top of every loop to prove the OS is still breathing
        Drivers::Watchdog::ResetTimer();

        led.Toggle();
        Core::DelayMs(500);
        console.Write("[HEARTBEAT] Loop OK.\r\n");

        if (console.HasData()) {
            const char incoming = console.Read(0).value_or('\0');
            
            // Artificial fault trigger: If you press 'X', the loop intentionally hangs
            if (incoming == 'X') {
                console.Write("[FAULT] Intentionally freezing the CPU. Goodbye.\r\n");
                
                // The watchdog will count down to 0 here and physically reset the STM32
                while (true) {
                    asm volatile("nop"); 
                }
            }
        }
    }

    return 0;
}