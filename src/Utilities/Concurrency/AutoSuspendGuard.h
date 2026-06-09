#pragma once

#include <atomic>

namespace QGC {

/// \brief RAII guard for an atomic suspend flag: sets on construction, clears on destruction.
///
/// Movable so it can live in moved-from owner objects (e.g. PendingOp variants).
///
class AutoSuspendGuard
{
public:
    explicit AutoSuspendGuard(std::atomic<bool>& flag) : _flag(&flag)
    {
        _flag->store(true, std::memory_order_release);
    }

    AutoSuspendGuard(AutoSuspendGuard&& other) noexcept : _flag(other._flag) { other._flag = nullptr; }

    AutoSuspendGuard& operator=(AutoSuspendGuard&& other) noexcept
    {
        if (this != &other) {
            release();
            _flag = other._flag;
            other._flag = nullptr;
        }
        return *this;
    }

    AutoSuspendGuard(const AutoSuspendGuard&) = delete;
    AutoSuspendGuard& operator=(const AutoSuspendGuard&) = delete;

    ~AutoSuspendGuard() { release(); }

private:
    void release() noexcept
    {
        if (_flag) {
            _flag->store(false, std::memory_order_release);
            _flag = nullptr;
        }
    }

    std::atomic<bool>* _flag;
};

}  // namespace QGC
