#pragma once

#include <QtCore/QObject>

#include "GPSBaseReference.h"
#include "GPSBaseStationFactGroup.h"
#include "GPSReceiverState.h"

/// Survey and fixed-base projection; the accepted state and Facts must outlive this object.
class GPSBaseStationState : public QObject
{
    Q_OBJECT

public:
    explicit GPSBaseStationState(GPSReceiverState& state, GPSBaseStationFactGroup& facts, QObject* parent = nullptr);
    ~GPSBaseStationState() override;

    GPSBaseReference reference() const { return _state.reference(); }

signals:
    void referenceChanged();

private:
    void _project();

    GPSReceiverState& _state;
    GPSBaseStationFactGroup& _facts;
    quint64 _revision = 0;
};
