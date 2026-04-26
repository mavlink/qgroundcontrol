#pragma once

#include <QtCore/QObject>
#include <QtCore/qmetatype.h>
#include <QtQmlIntegration/QtQmlIntegration>

/// Lightweight header exposing just the FSM's state enum.
///
/// VideoStream.h must advertise `fsmState` as a Q_PROPERTY and a signal
/// argument, but pulling in the full VideoStreamStateMachine.h drags in
/// QGCStateMachine.h → RequestMessageState.h → MAVLinkEnums.h and friends,
/// which the stand-alone VideoManagerModule QML target cannot satisfy. By
/// living in its own header the enum stays reachable from both worlds while
/// VideoStreamStateMachine.h simply pulls in this file and `using`-aliases
/// the enum into the class for ergonomic call sites.
namespace VideoStreamFsm {
Q_NAMESPACE
QML_ELEMENT

enum class State : quint8
{
    Idle,
    Starting,
    Connected,
    Streaming,
    Paused,
    Reconnecting,
    Stopping,
    Failed,
};
Q_ENUM_NS(State)
}  // namespace VideoStreamFsm
