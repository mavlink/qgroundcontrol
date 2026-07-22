#include "OnboardLogSession.h"

quint64 OnboardLogSessionBase::beginSession(bool active)
{
    ++_generation;
    _state = active ? State::Active : State::Idle;
    return _generation;
}

void OnboardLogSessionBase::invalidateSession()
{
    ++_generation;
}

void OnboardLogSessionBase::beginCancellation()
{
    if (active()) {
        _state = State::Canceling;
    }
}

void OnboardLogSessionBase::finishSession()
{
    _state = State::Idle;
}

bool OnboardLogSessionBase::isCurrentGeneration(quint64 generation) const
{
    return (generation == _generation) && active() && !canceling();
}
