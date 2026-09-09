#pragma once

#include "GPSBaseStationFactGroup.h"
#include "GPSPositionFactGroup.h"

class GPSReceiverFactGroup : public GPSPositionFactGroup
{
    Q_OBJECT
    Q_PROPERTY(GPSBaseStationFactGroup* rtk READ rtk CONSTANT)
    Q_PROPERTY(Fact* connected READ connected CONSTANT)
    Q_PROPERTY(Fact* numSatellites READ numSatellites CONSTANT)
    Q_PROPERTY(Fact* numSatellitesUsed READ numSatellitesUsed CONSTANT)
    Q_PROPERTY(Fact* lastError READ lastError CONSTANT)

public:
    explicit GPSReceiverFactGroup(QObject* parent = nullptr);
    ~GPSReceiverFactGroup();

    Fact* connected() { return &_connectedFact; }

    Fact* numSatellites() { return count(); }

    Fact* numSatellitesUsed() { return &_numSatellitesUsedFact; }

    Fact* lastError() { return &_lastErrorFact; }

    GPSBaseStationFactGroup* rtk() { return &_rtk; }

private:
    GPSBaseStationFactGroup _rtk;
    Fact _connectedFact = Fact(0, QStringLiteral("connected"), FactMetaData::valueTypeBool);
    Fact _numSatellitesUsedFact = Fact(0, QStringLiteral("numSatellitesUsed"), FactMetaData::valueTypeInt32);
    Fact _lastErrorFact = Fact(0, QStringLiteral("lastError"), FactMetaData::valueTypeUint32);
};
