#pragma once

#include <functional>
#include <memory>

#include <QtCore/QtGlobal>

/// Cooperative cancellation of a receiver session, shared by the thread that stops the session and the worker that
/// runs it. It stands in for std::stop_token, which libc++ before LLVM 20 offers only with -fexperimental-library.
class GPSCancelToken
{
public:
    /// A token that is never cancelled.
    GPSCancelToken() = default;

    /// Safe to call from any thread.
    [[nodiscard]] bool isCancelled() const;

private:
    friend class GPSCancelSource;
    friend class GPSCancelCallback;

    struct State;

    explicit GPSCancelToken(std::shared_ptr<State> state);

    std::shared_ptr<State> _state;
};

/// Issues tokens and cancels them all at once. Copies share the cancellation.
class GPSCancelSource
{
public:
    GPSCancelSource();

    [[nodiscard]] GPSCancelToken token() const;

    /// Cancels every token of this source and runs their callbacks on this thread before returning. Returns true only
    /// for the call that cancelled. Safe to call from any thread.
    bool cancel();

    [[nodiscard]] bool isCancelled() const;

private:
    std::shared_ptr<GPSCancelToken::State> _state;
};

/// Runs @a callback on the cancelling thread when @a token is cancelled, or at once on this thread if it already is.
/// Destruction waits for a running invocation, so the callback may use objects that outlive this one. It must not be
/// destroyed from a callback of the same token.
class GPSCancelCallback
{
public:
    GPSCancelCallback(const GPSCancelToken& token, std::function<void()> callback);
    ~GPSCancelCallback();

    Q_DISABLE_COPY(GPSCancelCallback)

private:
    std::shared_ptr<GPSCancelToken::State> _state;
    std::function<void()> _callback;
};
