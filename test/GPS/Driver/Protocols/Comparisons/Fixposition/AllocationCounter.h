#pragma once

#include <cstddef>

namespace AllocationCounter {
struct Counts
{
    std::size_t calls = 0;
    std::size_t bytes = 0;
    std::size_t largest = 0;
};

/// Counts this thread's C++ allocations only; direct malloc/realloc and library-internal allocators are not tracked.
void start();
Counts stop();
}  // namespace AllocationCounter
