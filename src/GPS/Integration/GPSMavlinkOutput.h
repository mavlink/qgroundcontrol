#pragma once

#include <QtCore/QHash>
#include <QtCore/QObject>

#include <memory>

#include "RTCMMavlink.h"

class LinkInterface;

/// Application adapter: snapshot distinct primary links and identify each connection lifetime.
class GPSMavlinkOutput : public QObject
{
    Q_OBJECT

public:
    explicit GPSMavlinkOutput(QObject* parent = nullptr);
    ~GPSMavlinkOutput() override;
    QList<RTCMMavlink::Output> outputs();

private:
    struct Connection
    {
        std::weak_ptr<LinkInterface> link;
        quint64 session = 0;
    };

    QHash<LinkInterface*, Connection> _connections;
    quint64 _nextSession = 0;
};
