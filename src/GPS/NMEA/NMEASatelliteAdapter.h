#pragma once

#include <QtCore/QIODevice>
#include <QtCore/QMap>
#include <QtCore/QPointer>
#include <QtCore/QSet>

#include "GPSObservation.h"
#include "GPSRuntimeScheduler.h"

/// Assembles multipart/multisignal NMEA satellite epochs into independent constellation reports.
class NMEASatelliteAdapter : public QObject
{
    Q_OBJECT

public:
    explicit NMEASatelliteAdapter(QIODevice* source, QObject* parent = nullptr,
                                  GPSRuntimeScheduler* scheduler = nullptr);
    ~NMEASatelliteAdapter() override;
    void close();

    bool isOpen() const { return _open; }

signals:
    void observationReceived(const GPSSatelliteObservation& observation);

private:
    struct SignalReport
    {
        int messageCount = 0;
        int satelliteCount = 0;
        int nextMessage = 1;
        quint64 receivedAtUs = 0;
        QList<GPSSatellite> satellites;
        bool complete() const { return messageCount > 0 && nextMessage == messageCount + 1; }
    };

    struct UsedReport
    {
        quint64 receivedAtUs = 0;
        QSet<int> ids;
    };

    void _readAvailable();
    void _parseSentence(const QByteArray& sentence, quint64 receivedAtUs);
    void _flush();
    void _deliver();

    QPointer<QIODevice> _source;
    GPSRuntimeScheduler* _scheduler;
    GPSRuntimeScheduler::TaskId _idleTask = 0;
    GPSRuntimeScheduler::TaskId _batchTask = 0;
    GPSRuntimeScheduler::TaskId _deliveryTask = 0;
    GPSRuntimeScheduler::TaskId _readTask = 0;
    QMap<GPSSatellite::Constellation, QMap<int, SignalReport>> _reports;
    QMap<GPSSatellite::Constellation, UsedReport> _inUse;
    QByteArray _epochTime;
    QList<GPSSatelliteObservation> _pending;
    bool _open = true;
};
