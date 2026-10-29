#pragma once

#include <QtCore/QObject>
#include <QtCore/QTimer>

#include "MAVLinkMessageType.h"

class Vehicle;

/// One bounded opaque-device transaction, scoped to a single vehicle connection.
class AmovUomTransport final : public QObject
{
    Q_OBJECT
public:
    explicit AmovUomTransport(Vehicle* vehicle, QObject* parent = nullptr);
    bool send(quint8 operation, quint8 node, const QByteArray& payload);
    void receive(const mavlink_message_t& message);
    void reset();

    bool busy() const { return _pending != 0; }

signals:
    void completed(quint8 operation, quint8 node, const QByteArray& payload);
    void failed(int status, const QString& detail);

private:
    Vehicle* _vehicle;
    QTimer _deadline;
    quint64 _session = 0;
    quint32 _nextRequest = 0;
    quint32 _pending = 0;
    quint8 _operation = 0;
    quint8 _node = 0;
};
