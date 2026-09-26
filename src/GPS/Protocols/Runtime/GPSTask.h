#pragma once

#include <coroutine>
#include <exception>
#include <optional>
#include <type_traits>
#include <utility>

template <typename T = void>
class GPSTask;

namespace GPSTaskDetail {

struct FinalAwaiter
{
    bool await_ready() const noexcept { return false; }

    template <typename Promise>
    std::coroutine_handle<> await_suspend(std::coroutine_handle<Promise> handle) const noexcept
    {
        // A task finishing inside its awaiter's await_suspend() returns there; see GPSTask::Awaiter.
        auto& promise = handle.promise();
        return promise.runningInline ? std::noop_coroutine() : promise.continuation;
    }

    void await_resume() const noexcept {}
};

struct PromiseBase
{
    std::suspend_always initial_suspend() const noexcept { return {}; }

    FinalAwaiter final_suspend() const noexcept { return {}; }

    [[noreturn]] void unhandled_exception() const noexcept { std::terminate(); }

    /// Resumed by symmetric transfer when the task finishes after suspending; a driver-started task returns to its
    /// driver.
    std::coroutine_handle<> continuation = std::noop_coroutine();
    bool started = false;
    bool runningInline = false;
};

template <typename T>
struct Promise : PromiseBase
{
    GPSTask<T> get_return_object() noexcept;

    void return_value(T value) noexcept(std::is_nothrow_move_constructible_v<T>) { result.emplace(std::move(value)); }

    std::optional<T> result;
};

template <>
struct Promise<void> : PromiseBase
{
    GPSTask<void> get_return_object() noexcept;

    void return_void() const noexcept {}
};

}  // namespace GPSTaskDetail

/// Lazily started coroutine that owns its frame. Awaiting a task starts it. A task that finishes without suspending
/// lets its awaiter continue directly; one that suspended resumes its awaiter by symmetric transfer when it finishes.
/// Neither depends on the compiler turning symmetric transfer into a tail call, so loops over synchronously finishing
/// tasks run in constant stack, including unoptimised GCC builds. Destroying a task destroys its frame and the frames
/// of the child tasks it is suspended in, which is how a suspended operation is cancelled. An exception escaping the
/// body terminates. Await a task in the full-expression that creates it: parameters taken by reference or view must
/// outlive the task.
///
/// An empty GPSTask<void> (default-constructed, or ready()) is an already finished no-op, so a non-coroutine override
/// can return one. GPSTask<T> has no empty state except after a move; awaiting or reading a moved-from task
/// terminates.
template <typename T>
class [[nodiscard]] GPSTask
{
public:
    using promise_type = GPSTaskDetail::Promise<T>;
    using Handle = std::coroutine_handle<promise_type>;

    GPSTask() noexcept
        requires std::is_void_v<T>
    = default;

    /// A finished task with nothing to do.
    [[nodiscard]] static GPSTask ready() noexcept
        requires std::is_void_v<T>
    {
        return {};
    }

    GPSTask(GPSTask&& other) noexcept
        : _handle(std::exchange(other._handle, {}))
    {}

    GPSTask& operator=(GPSTask&& other) noexcept
    {
        if (this != &other) {
            reset();
            _handle = std::exchange(other._handle, {});
        }
        return *this;
    }

    GPSTask(const GPSTask&) = delete;
    GPSTask& operator=(const GPSTask&) = delete;

    ~GPSTask() { reset(); }

    [[nodiscard]] bool valid() const noexcept { return static_cast<bool>(_handle); }

    [[nodiscard]] bool done() const noexcept { return !_handle || _handle.done(); }

    /// Runs a task that nothing awaits until its first suspension. For drivers only: later resumption goes through
    /// the handle the task suspended on.
    void start()
    {
        if (_handle && !_handle.promise().started) {
            _handle.promise().started = true;
            _handle.resume();
        }
    }

    /// Destroys the frame, cancelling the task if it is suspended.
    void reset() noexcept
    {
        if (_handle) {
            std::exchange(_handle, {}).destroy();
        }
    }

    /// Moves out the value of a finished task; terminates when the task has not finished or holds no value.
    T result()
    {
        if constexpr (std::is_void_v<T>) {
            if (_handle && !_handle.done()) {
                std::terminate();
            }
        } else {
            if (!_handle || !_handle.done()) {
                std::terminate();
            }
            return std::move(*_handle.promise().result);
        }
    }

    auto operator co_await() & noexcept { return Awaiter{_handle}; }

    auto operator co_await() && noexcept { return Awaiter{_handle}; }

private:
    friend promise_type;

    explicit GPSTask(Handle handle) noexcept
        : _handle(handle)
    {}

    struct Awaiter
    {
        Handle handle;

        bool await_ready() const noexcept { return !handle || handle.done(); }

        /// Runs the task until it suspends or finishes. @return false to continue the awaiter when it finished.
        bool await_suspend(std::coroutine_handle<> awaiting) noexcept
        {
            auto& promise = handle.promise();
            promise.continuation = awaiting;
            promise.started = true;
            promise.runningInline = true;
            handle.resume();
            promise.runningInline = false;
            return !handle.done();
        }

        T await_resume()
        {
            if constexpr (!std::is_void_v<T>) {
                if (!handle) {
                    std::terminate();
                }
                return std::move(*handle.promise().result);
            }
        }
    };

    Handle _handle;
};

namespace GPSTaskDetail {

template <typename T>
GPSTask<T> Promise<T>::get_return_object() noexcept
{
    return GPSTask<T>(std::coroutine_handle<Promise<T>>::from_promise(*this));
}

inline GPSTask<void> Promise<void>::get_return_object() noexcept
{
    return GPSTask<void>(std::coroutine_handle<Promise<void>>::from_promise(*this));
}

}  // namespace GPSTaskDetail
