#pragma once

#include <QtCore/QEvent>
#include <QtCore/QString>
#include <QtCore/QVariant>

/// Custom event for QGCStateMachine delayed/scheduled events
class QGCStateMachineEvent : public QEvent
{
public:
    static const QEvent::Type EventType;

    /// Create a named event with optional data
    /// @param name Event name for identification
    /// @param data Optional payload data
    explicit QGCStateMachineEvent(const QString& name, const QVariant& data = QVariant());

    QString name() const { return _name; }
    QVariant data() const { return _data; }

private:
    QString _name;
    QVariant _data;
};
