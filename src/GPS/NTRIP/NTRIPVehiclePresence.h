#pragma once

#include <QtCore/QObject>

/// Whether a vehicle is connected, for an NTRIP connection that runs only while one is. A subclass reports it; this
/// class knows no vehicle, so that the connection is tested without the application.
class NTRIPVehiclePresence : public QObject
{
    Q_OBJECT

public:
    explicit NTRIPVehiclePresence(QObject* parent = nullptr)
        : QObject(parent)
    {}

    bool present() const { return _present; }

signals:
    void presentChanged(bool present);

protected:
    /// Emits presentChanged() only when @a present differs from the current state.
    void _setPresent(bool present)
    {
        if (_present != present) {
            _present = present;
            emit presentChanged(present);
        }
    }

private:
    bool _present = false;
};
