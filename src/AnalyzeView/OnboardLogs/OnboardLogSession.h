#pragma once

#include <QtCore/QtGlobal>
#include <cstdint>

/// Shared lifecycle and stale-callback generation tracking for onboard-log operations.
///
/// Session implementations remain value types with protocol-specific begin,
/// cancellation, and completion APIs. This base only owns the lifecycle
/// invariants common to those implementations.
class OnboardLogSessionBase
{
    Q_DISABLE_COPY_MOVE(OnboardLogSessionBase)

public:
    quint64 generation() const { return _generation; }

    bool active() const { return _state != State::Idle; }

    bool canceling() const { return _state == State::Canceling; }

protected:
    OnboardLogSessionBase() = default;
    ~OnboardLogSessionBase() = default;

    [[nodiscard]] quint64 beginSession(bool active = true);
    void invalidateSession();
    void beginCancellation();
    void finishSession();

    bool isCurrentGeneration(quint64 generation) const;

private:
    enum class State : uint8_t
    {
        Idle,
        Active,
        Canceling,
    };

    quint64 _generation = 0;
    State _state = State::Idle;
};
