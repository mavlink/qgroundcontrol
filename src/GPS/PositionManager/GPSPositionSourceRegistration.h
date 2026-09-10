#pragma once

#include <QtCore/QPointer>

class QGCPositionManager;

/// A source registration is retired only by its own token, never by a superseded owner.
class GPSPositionSourceRegistration
{
    friend class QGCPositionManager;

public:
    GPSPositionSourceRegistration();
    ~GPSPositionSourceRegistration();
    GPSPositionSourceRegistration(GPSPositionSourceRegistration&& other) noexcept;
    GPSPositionSourceRegistration& operator=(GPSPositionSourceRegistration&& other) noexcept;
    void reset();

    explicit operator bool() const { return !_manager.isNull() && _token != 0; }

    GPSPositionSourceRegistration(const GPSPositionSourceRegistration&) = delete;
    GPSPositionSourceRegistration& operator=(const GPSPositionSourceRegistration&) = delete;

private:
    GPSPositionSourceRegistration(QGCPositionManager* manager, int kind, quint64 token);
    void _swap(GPSPositionSourceRegistration& other) noexcept;
    QPointer<QGCPositionManager> _manager;
    int _kind = 0;
    quint64 _token = 0;
};
