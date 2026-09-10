#pragma once

#include <QtCore/QIODevice>
#include <QtCore/QPointer>

#include "GPSObservation.h"
#include "GPSRuntimeScheduler.h"
#include "GPSScheduledTask.h"
#include "NMEASatelliteEpoch.h"
#include "NMEASentenceEnvelope.h"

/// Assembles multipart/multisignal NMEA satellite epochs into independent constellation reports.
class NMEASatelliteAdapter : public QObject
{
    Q_OBJECT

public:
    explicit NMEASatelliteAdapter(QIODevice* source, QObject* parent = nullptr,
                                  GPSRuntimeScheduler* scheduler = nullptr);
    ~NMEASatelliteAdapter() override;
    void close();
    void ingest(const NMEASentenceEnvelope& sentence);

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
    QPointer<GPSRuntimeScheduler> _scheduler;
    GPSScheduledTask _idleTask;
    GPSScheduledTask _batchTask;
    GPSScheduledTask _deliveryTask;
    GPSScheduledTask _readTask;
    NMEA::SatelliteAssembler _assembler;
    QList<GPSSatelliteObservation> _pending;
    bool _open = true;
};
