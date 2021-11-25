/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "FactGroup.h"
#include "QGCMAVLink.h"

class Vehicle;

class VehicleDistanceSensorFactGroup : public FactGroup
{
    Q_OBJECT

public:
    VehicleDistanceSensorFactGroup(QObject* parent = nullptr);

    Q_PROPERTY(Fact* rotationNone       READ rotationNone       CONSTANT)
    Q_PROPERTY(Fact* rotationYaw45      READ rotationYaw45      CONSTANT)
    Q_PROPERTY(Fact* rotationYaw90      READ rotationYaw90      CONSTANT)
    Q_PROPERTY(Fact* rotationYaw135     READ rotationYaw135     CONSTANT)
    Q_PROPERTY(Fact* rotationYaw180     READ rotationYaw180     CONSTANT)
    Q_PROPERTY(Fact* rotationYaw225     READ rotationYaw225     CONSTANT)
    Q_PROPERTY(Fact* rotationYaw270     READ rotationYaw270     CONSTANT)
    Q_PROPERTY(Fact* rotationYaw315     READ rotationYaw315     CONSTANT)
    Q_PROPERTY(Fact* rotationPitch90    READ rotationPitch90    CONSTANT)
    Q_PROPERTY(Fact* rotationPitch270   READ rotationPitch270   CONSTANT)
    Q_PROPERTY(Fact* minDistance        READ minDistance        CONSTANT)
    Q_PROPERTY(Fact* maxDistance        READ maxDistance        CONSTANT)

    Fact* rotationNone      () { return &_rotationNoneFact; }
    Fact* rotationYaw45     () { return &_rotationYaw45Fact; }
    Fact* rotationYaw90     () { return &_rotationYaw90Fact; }
    Fact* rotationYaw135    () { return &_rotationYaw135Fact; }
    Fact* rotationYaw180    () { return &_rotationYaw180Fact; }
    Fact* rotationYaw225    () { return &_rotationYaw225Fact; }
    Fact* rotationYaw270    () { return &_rotationYaw270Fact; }
    Fact* rotationYaw315    () { return &_rotationYaw315Fact; }
    Fact* rotationPitch90   () { return &_rotationPitch90Fact; }
    Fact* rotationPitch270  () { return &_rotationPitch270Fact; }
    Fact* minDistance       () { return &_minDistanceFact; }
    Fact* maxDistance       () { return &_maxDistanceFact; }

    // Overrides from FactGroup
    void handleMessage(Vehicle* vehicle, mavlink_message_t& message) override;

    static constexpr const char* _rotationNoneFactName      = "rotationNone";
    static constexpr const char* _rotationYaw45FactName     = "rotationYaw45";
    static constexpr const char* _rotationYaw90FactName     = "rotationYaw90";
    static constexpr const char* _rotationYaw135FactName    = "rotationYaw135";
    static constexpr const char* _rotationYaw180FactName    = "rotationYaw180";
    static constexpr const char* _rotationYaw225FactName    = "rotationYaw225";
    static constexpr const char* _rotationYaw270FactName    = "rotationYaw270";
    static constexpr const char* _rotationYaw315FactName    = "rotationYaw315";
    static constexpr const char* _rotationPitch90FactName   = "rotationPitch90";
    static constexpr const char* _rotationPitch270FactName  = "rotationPitch270";
    static constexpr const char* _minDistanceFactName       = "minDistance";
    static constexpr const char* _maxDistanceFactName       = "maxDistance";

private:
    Fact _rotationNoneFact;
    Fact _rotationYaw45Fact;
    Fact _rotationYaw90Fact;
    Fact _rotationYaw135Fact;
    Fact _rotationYaw180Fact;
    Fact _rotationYaw225Fact;
    Fact _rotationYaw270Fact;
    Fact _rotationYaw315Fact;
    Fact _rotationPitch90Fact;
    Fact _rotationPitch270Fact;
    Fact _minDistanceFact;
    Fact _maxDistanceFact;
};
