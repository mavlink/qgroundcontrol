#pragma once

#include <cstddef>

namespace QGCTest {
namespace detail {
void recordAllocation(std::size_t size) noexcept;
bool failAllocation() noexcept;
}  // namespace detail

/// Counts C++ allocation requests on this thread, including failed requests and nested scopes.
/// Direct malloc/realloc calls are not counted. Read the result before leaving the scope;
/// keep Qt assertions and reporting outside the measured scope.
class AllocationTracker
{
public:
    struct Counts
    {
        std::size_t calls = 0;
        std::size_t bytes = 0;
        std::size_t largest = 0;
    };

    AllocationTracker() noexcept;
    ~AllocationTracker();
    AllocationTracker(const AllocationTracker&) = delete;
    AllocationTracker& operator=(const AllocationTracker&) = delete;
    AllocationTracker(AllocationTracker&&) = delete;
    AllocationTracker& operator=(AllocationTracker&&) = delete;

    Counts counts() const noexcept { return _counts; }

private:
    friend void detail::recordAllocation(std::size_t size) noexcept;
    AllocationTracker* _previous;
    Counts _counts;
};

/// Fail a bounded number of allocation attempts on this thread, without relying on host OOM behavior.
class AllocationFailureScope
{
public:
    explicit AllocationFailureScope(std::size_t failures) noexcept;
    ~AllocationFailureScope();
    AllocationFailureScope(const AllocationFailureScope&) = delete;
    AllocationFailureScope& operator=(const AllocationFailureScope&) = delete;
    AllocationFailureScope(AllocationFailureScope&&) = delete;
    AllocationFailureScope& operator=(AllocationFailureScope&&) = delete;

private:
    friend bool detail::failAllocation() noexcept;
    AllocationFailureScope* _previous;
    std::size_t _remaining;
};
}  // namespace QGCTest
