#pragma once

#include "FactGroup.h"

class GPSReceiverFactGroup : public FactGroup
{
    Q_OBJECT
    Q_PROPERTY(Fact* connected READ connected CONSTANT)
    Q_PROPERTY(Fact* numSatellites READ numSatellites CONSTANT)
    Q_PROPERTY(Fact* numSatellitesUsed READ numSatellitesUsed CONSTANT)
    Q_PROPERTY(Fact* lastError READ lastError CONSTANT)

public:
    explicit GPSReceiverFactGroup(QObject* parent = nullptr);
    ~GPSReceiverFactGroup();

    Fact* connected() { return &_connectedFact; }

    Fact* numSatellites() { return &_numSatellitesFact; }

    Fact* numSatellitesUsed() { return &_numSatellitesUsedFact; }

    Fact* lastError() { return &_lastErrorFact; }

private:
    Fact _connectedFact = Fact(0, QStringLiteral("connected"), FactMetaData::valueTypeBool);
    Fact _numSatellitesFact = Fact(0, QStringLiteral("numSatellites"), FactMetaData::valueTypeInt32);
    Fact _numSatellitesUsedFact = Fact(0, QStringLiteral("numSatellitesUsed"), FactMetaData::valueTypeInt32);
    Fact _lastErrorFact = Fact(0, QStringLiteral("lastError"), FactMetaData::valueTypeUint32);
};
