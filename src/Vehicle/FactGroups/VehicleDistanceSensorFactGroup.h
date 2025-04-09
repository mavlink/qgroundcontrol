/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "FactGroup.h"

class VehicleDistanceSensorFactGroup : public FactGroup
{
    Q_OBJECT
    Q_PROPERTY(Fact *rotationNone       READ rotationNone       CONSTANT)
    Q_PROPERTY(Fact *rotationYaw45      READ rotationYaw45      CONSTANT)
    Q_PROPERTY(Fact *rotationYaw90      READ rotationYaw90      CONSTANT)
    Q_PROPERTY(Fact *rotationYaw135     READ rotationYaw135     CONSTANT)
    Q_PROPERTY(Fact *rotationYaw180     READ rotationYaw180     CONSTANT)
    Q_PROPERTY(Fact *rotationYaw225     READ rotationYaw225     CONSTANT)
    Q_PROPERTY(Fact *rotationYaw270     READ rotationYaw270     CONSTANT)
    Q_PROPERTY(Fact *rotationYaw315     READ rotationYaw315     CONSTANT)
    Q_PROPERTY(Fact *rotationPitch90    READ rotationPitch90    CONSTANT)
    Q_PROPERTY(Fact *rotationPitch270   READ rotationPitch270   CONSTANT)
    Q_PROPERTY(Fact *minDistance        READ minDistance        CONSTANT)
    Q_PROPERTY(Fact *maxDistance        READ maxDistance        CONSTANT)

public:
    explicit VehicleDistanceSensorFactGroup(QObject *parent = nullptr);

    Fact *rotationNone() { return &_rotationNoneFact; }
    Fact *rotationYaw45() { return &_rotationYaw45Fact; }
    Fact *rotationYaw90() { return &_rotationYaw90Fact; }
    Fact *rotationYaw135() { return &_rotationYaw135Fact; }
    Fact *rotationYaw180() { return &_rotationYaw180Fact; }
    Fact *rotationYaw225() { return &_rotationYaw225Fact; }
    Fact *rotationYaw270() { return &_rotationYaw270Fact; }
    Fact *rotationYaw315() { return &_rotationYaw315Fact; }
    Fact *rotationPitch90() { return &_rotationPitch90Fact; }
    Fact *rotationPitch270() { return &_rotationPitch270Fact; }
    Fact *minDistance() { return &_minDistanceFact; }
    Fact *maxDistance() { return &_maxDistanceFact; }

    // Overrides from FactGroup
    void handleMessage(Vehicle *vehicle, const mavlink_message_t &message) final;

private:
    Fact _rotationNoneFact = Fact(0, QStringLiteral("rotationNone"), FactMetaData::valueTypeDouble);
    Fact _rotationYaw45Fact = Fact(0, QStringLiteral("rotationYaw45"), FactMetaData::valueTypeDouble);
    Fact _rotationYaw90Fact = Fact(0, QStringLiteral("rotationYaw90"), FactMetaData::valueTypeDouble);
    Fact _rotationYaw135Fact = Fact(0, QStringLiteral("rotationYaw135"), FactMetaData::valueTypeDouble);
    Fact _rotationYaw180Fact = Fact(0, QStringLiteral("rotationYaw180"), FactMetaData::valueTypeDouble);
    Fact _rotationYaw225Fact = Fact(0, QStringLiteral("rotationYaw225"), FactMetaData::valueTypeDouble);
    Fact _rotationYaw270Fact = Fact(0, QStringLiteral("rotationYaw270"), FactMetaData::valueTypeDouble);
    Fact _rotationYaw315Fact = Fact(0, QStringLiteral("rotationYaw315"), FactMetaData::valueTypeDouble);
    Fact _rotationPitch90Fact = Fact(0, QStringLiteral("rotationPitch90"), FactMetaData::valueTypeDouble);
    Fact _rotationPitch270Fact = Fact(0, QStringLiteral("rotationPitch270"), FactMetaData::valueTypeDouble);
    Fact _minDistanceFact = Fact(0, QStringLiteral("minDistance"), FactMetaData::valueTypeDouble);
    Fact _maxDistanceFact = Fact(0, QStringLiteral("maxDistance"), FactMetaData::valueTypeDouble);
};
