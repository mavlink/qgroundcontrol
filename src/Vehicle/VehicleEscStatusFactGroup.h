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

class VehicleEscStatusFactGroup : public FactGroup
{
    Q_OBJECT

public:
    VehicleEscStatusFactGroup(QObject* parent = nullptr);

    Q_PROPERTY(Fact* index              READ index              CONSTANT)

    Q_PROPERTY(Fact* rpmFirst           READ rpmFirst           CONSTANT)
    Q_PROPERTY(Fact* rpmSecond          READ rpmSecond          CONSTANT)
    Q_PROPERTY(Fact* rpmThird           READ rpmThird           CONSTANT)
    Q_PROPERTY(Fact* rpmFourth          READ rpmFourth          CONSTANT)

    Q_PROPERTY(Fact* currentFirst       READ currentFirst       CONSTANT)
    Q_PROPERTY(Fact* currentSecond      READ currentSecond      CONSTANT)
    Q_PROPERTY(Fact* currentThird       READ currentThird       CONSTANT)
    Q_PROPERTY(Fact* currentFourth      READ currentFourth      CONSTANT)

    Q_PROPERTY(Fact* voltageFirst       READ voltageFirst       CONSTANT)
    Q_PROPERTY(Fact* voltageSecond      READ voltageSecond      CONSTANT)
    Q_PROPERTY(Fact* voltageThird       READ voltageThird       CONSTANT)
    Q_PROPERTY(Fact* voltageFourth      READ voltageFourth      CONSTANT)

    Fact* index                         () { return &_indexFact; }

    Fact* rpmFirst                      () { return &_rpmFirstFact; }
    Fact* rpmSecond                     () { return &_rpmSecondFact; }
    Fact* rpmThird                      () { return &_rpmThirdFact; }
    Fact* rpmFourth                     () { return &_rpmFourthFact; }

    Fact* currentFirst                  () { return &_currentFirstFact; }
    Fact* currentSecond                 () { return &_currentSecondFact; }
    Fact* currentThird                  () { return &_currentThirdFact; }
    Fact* currentFourth                 () { return &_currentFourthFact; }

    Fact* voltageFirst                  () { return &_voltageFirstFact; }
    Fact* voltageSecond                 () { return &_voltageSecondFact; }
    Fact* voltageThird                  () { return &_voltageThirdFact; }
    Fact* voltageFourth                 () { return &_voltageFourthFact; }

    // Overrides from FactGroup
    void handleMessage(Vehicle* vehicle, mavlink_message_t& message) override;

    static constexpr const char* _indexFactName = "index";

    static constexpr const char* _rpmFirstFactName  = "rpm1";
    static constexpr const char* _rpmSecondFactName = "rpm2";
    static constexpr const char* _rpmThirdFactName  = "rpm3";
    static constexpr const char* _rpmFourthFactName = "rpm4";

    static constexpr const char* _currentFirstFactName  = "current1";
    static constexpr const char* _currentSecondFactName = "current2";
    static constexpr const char* _currentThirdFactName  = "current3";
    static constexpr const char* _currentFourthFactName = "current4";

    static constexpr const char* _voltageFirstFactName  = "voltage1";
    static constexpr const char* _voltageSecondFactName = "voltage2";
    static constexpr const char* _voltageThirdFactName  = "voltage3";
    static constexpr const char* _voltageFourthFactName = "voltage4";
    
private:
    Fact _indexFact;

    Fact _rpmFirstFact;
    Fact _rpmSecondFact;
    Fact _rpmThirdFact;
    Fact _rpmFourthFact;

    Fact _currentFirstFact;
    Fact _currentSecondFact;
    Fact _currentThirdFact;
    Fact _currentFourthFact;

    Fact _voltageFirstFact;
    Fact _voltageSecondFact;
    Fact _voltageThirdFact;
    Fact _voltageFourthFact;
};
