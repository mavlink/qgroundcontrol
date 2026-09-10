#pragma once

#include <QtCore/QIODevice>
#include <QtCore/QMap>
#include <QtCore/QPointer>
#include <QtCore/QSet>

#include "GPSObservation.h"
#include "GPSRuntimeScheduler.h"
#include "NMEASatelliteEpoch.h"

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
    void _readAvailable();
    void _parseSentence(const QByteArray& sentence, quint64 receivedAtUs);
    void _flush();
    void _deliver();
    void _queue(NMEA::SatelliteEpoch epoch);

    QPointer<QIODevice> _source;
    GPSRuntimeScheduler* _scheduler;
    GPSRuntimeScheduler::TaskId _idleTask = 0;
    GPSRuntimeScheduler::TaskId _batchTask = 0;
    GPSRuntimeScheduler::TaskId _deliveryTask = 0;
    GPSRuntimeScheduler::TaskId _readTask = 0;
    NMEA::SatelliteAssembler _assembler;
    QList<GPSSatelliteObservation> _pending;
    bool _open = true;
};
