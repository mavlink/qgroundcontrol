#pragma once

#include <QtCore/QPointer>

class QGCPositionManager;

/// A source registration is retired only by its own token, never by a superseded owner.
class GPSPositionSourceRegistration
{
    friend class QGCPositionManager;

public:
    ~GPSPositionSourceRegistration();
    GPSPositionSourceRegistration(const GPSPositionSourceRegistration&) = delete;
    GPSPositionSourceRegistration& operator=(const GPSPositionSourceRegistration&) = delete;

private:
    GPSPositionSourceRegistration(QGCPositionManager* manager, int kind, quint64 token);
    QPointer<QGCPositionManager> _manager;
    int _kind;
    quint64 _token;
};
