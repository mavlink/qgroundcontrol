#include "GPSCorrectionStatus.h"

#include <utility>

#include "Fact.h"
#include "GPSCorrectionManager.h"
#include "GPSRtk.h"
#include "NTRIPManager.h"

GPSCorrectionStatus::GPSCorrectionStatus(GPSCorrectionManager* corrections, NTRIPManager* ntrip, GPSRtk* rtk,
                                         Fact* udpInputEnabled, QObject* parent)
    : QObject(parent)
    , _corrections(corrections)
    , _ntrip(ntrip)
    , _rtk(rtk)
    , _udpInputEnabled(udpInputEnabled)
{
    if (_corrections) {
        (void) connect(_corrections, &GPSCorrectionManager::sourceInstancesChanged, this,
                       &GPSCorrectionStatus::_update);
    }
    if (_ntrip) {
        (void) connect(_ntrip, &NTRIPManager::connectionStatusChanged, this, &GPSCorrectionStatus::_update);
    }
    if (_rtk) {
        (void) connect(_rtk, &GPSRtk::receiverChanged, this, &GPSCorrectionStatus::_update);
    }
    if (_udpInputEnabled) {
        (void) connect(_udpInputEnabled, &Fact::rawValueChanged, this, &GPSCorrectionStatus::_update);
    }
    _update();
}

void GPSCorrectionStatus::_update()
{
    using State = GPSManager::CorrectionState;
    auto state = State::Inactive;
    if (_corrections && _corrections->hasSelectedStream()) {
        state = State::Fresh;
    } else if ((_ntrip && _ntrip->connectionStatus() != NTRIPManager::ConnectionStatus::Disconnected) ||
               (_udpInputEnabled && _udpInputEnabled->rawValue().toBool()) ||
               (_rtk && _rtk->hasReceiver() && _rtk->activeRole() != GPSRtk::PositionOnly) ||
               (_corrections && !_corrections->sourceInstances().isEmpty())) {
        state = State::Waiting;
    }
    if (std::exchange(_state, state) != state) {
        emit stateChanged();
    }
}
