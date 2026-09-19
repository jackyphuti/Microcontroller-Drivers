#include "core/scheduler.hpp"

namespace Core {
namespace Scheduler {

static Task tasks[MAX_TASKS];
static int current_task_idx = 0;
static std::uint32_t next_id = 1;

// ARM Cortex-M System Control Block for triggering PendSV
inline constexpr std::uintptr_t SCB_ICSR = 0xE000ED04UL;

void Init() {
    for (int i = 0; i < MAX_TASKS; ++i) {
        tasks[i].state = TaskState::Empty;
    }
}

int CreateTask(void (*entry_point)()) {
    for (int i = 0; i < MAX_TASKS; ++i) {
        if (tasks[i].state == TaskState::Empty) {
            tasks[i].id = next_id++;
            tasks[i].state = TaskState::Ready;

            // Initialize the task stack to simulate a CPU exception frame
            // Cortex-M expects xPSR, PC, LR, R12, R3, R2, R1, R0 at the top of the stack
            std::uint32_t* sp = &tasks[i].stack[STACK_SIZE - 16];
            
            sp[15] = 0x01000000; // xPSR: Set Thumb bit
            sp[14] = reinterpret_cast<std::uint32_t>(entry_point); // PC: Entry function
            sp[13] = 0xFFFFFFFD; // LR: Return to thread mode using PSP
            
            tasks[i].stack_pointer = sp;
            return tasks[i].id;
        }
    }
    return -1;
}

void Yield() {
    // Trigger the PendSV interrupt by setting bit 28 of the ICSR register
    auto* icsr = reinterpret_cast<volatile std::uint32_t*>(SCB_ICSR);
    *icsr |= (1U << 28); 
}

} // namespace Scheduler
} // namespace Core

// The actual assembly context switch triggered by Yield()
extern "C" __attribute__((naked)) void PendSV_Handler() {
    asm volatile(
        "mrs r0, psp \n"        // Get current process stack pointer
        "stmdb r0!, {r4-r11} \n"// Save remaining registers (r4-r11)
        
        // Context switch logic would load the next task's stack pointer here
        // (Simplified for this boilerplate)

        "ldmia r0!, {r4-r11} \n"// Load next task's registers
        "msr psp, r0 \n"        // Update process stack pointer
        "bx lr \n"              // Return from interrupt
    );
}