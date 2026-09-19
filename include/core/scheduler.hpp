#pragma once

#include <cstdint>

namespace Core {
namespace Scheduler {

    inline constexpr int MAX_TASKS = 4;
    inline constexpr int STACK_SIZE = 1024; // 1KB per task

    enum class TaskState { Empty, Ready, Running };

    struct Task {
        std::uint32_t id;
        TaskState state;
        std::uint32_t* stack_pointer;
        std::uint32_t stack[STACK_SIZE];
    };

    void Init();
    int CreateTask(void (*entry_point)());
    void Yield();

} // namespace Scheduler
} // namespace Core