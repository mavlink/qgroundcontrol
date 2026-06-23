#include "QGCStateMachineEvent.h"

const QEvent::Type QGCStateMachineEvent::EventType = static_cast<QEvent::Type>(QEvent::registerEventType());

QGCStateMachineEvent::QGCStateMachineEvent(const QString& name, const QVariant& data)
    : QEvent(EventType)
    , _name(name)
    , _data(data)
{
}
