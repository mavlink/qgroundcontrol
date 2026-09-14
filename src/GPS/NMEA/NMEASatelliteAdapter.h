#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointer>

#include "GPSObservation.h"
#include "NMEASatelliteEpoch.h"
#include "NMEASentenceEnvelope.h"
#include "RuntimeScheduler.h"
#include "ScheduledTask.h"

/// Assembles multipart/multisignal NMEA satellite epochs into independent constellation reports.
class NMEASatelliteAdapter : public QObject
{
    Q_OBJECT

public:
    explicit NMEASatelliteAdapter(QObject* parent = nullptr, RuntimeScheduler* scheduler = nullptr);
    ~NMEASatelliteAdapter() override;
    void close();
    void ingest(const NMEASentenceEnvelope& sentence);

    bool isOpen() const { return _open; }

signals:
    void observationReceived(const GPSSatelliteObservation& observation);

private:
    void _flush();
    void _deliver();
    void _queue(NMEA::SatelliteEpoch epoch);

    QPointer<RuntimeScheduler> _scheduler;
    ScheduledTask _idleTask;
    ScheduledTask _batchTask;
    ScheduledTask _deliveryTask;
    NMEA::SatelliteAssembler _assembler;
    QList<GPSSatelliteObservation> _pending;
    bool _open = true;
};
