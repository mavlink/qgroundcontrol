#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointer>

#include "GPSManager.h"

class Fact;
class GPSCorrectionManager;
class GPSRtk;
class NTRIPManager;

/// Derives GPSManager::CorrectionState from the correction router, NTRIP, the UDP input setting and the receiver.
class GPSCorrectionStatus : public QObject
{
    Q_OBJECT

public:
    GPSCorrectionStatus(GPSCorrectionManager* corrections, NTRIPManager* ntrip, GPSRtk* rtk, Fact* udpInputEnabled,
                        QObject* parent = nullptr);

    GPSManager::CorrectionState state() const { return _state; }

signals:
    void stateChanged();

private:
    void _update();

    // The inputs are siblings of this object and may be destroyed first.
    QPointer<GPSCorrectionManager> _corrections;
    QPointer<NTRIPManager> _ntrip;
    QPointer<GPSRtk> _rtk;
    QPointer<Fact> _udpInputEnabled;
    GPSManager::CorrectionState _state = GPSManager::CorrectionState::Inactive;
};
