#pragma once

#include <QtCore/QObject>

/// Tracks whether any network is reachable, as reported by the platform's QNetworkInformation backend. Without a
/// backend, or once the backend is gone, the network is assumed to be available.
class QGCNetworkAvailabilityMonitor : public QObject
{
    Q_OBJECT

public:
    explicit QGCNetworkAvailabilityMonitor(QObject* parent = nullptr);

    bool available() const { return _available; }

signals:
    void availableChanged(bool available);

protected:
    /// For subclasses that report availability themselves: skips the platform backend and starts at @a available.
    QGCNetworkAvailabilityMonitor(bool available, QObject* parent);

    /// Emits availableChanged() only when @a available differs from the current state.
    void _setAvailable(bool available);

private:
    bool _available = true;
};
