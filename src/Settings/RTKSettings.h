/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "SettingsGroup.h"

class RTKSettings : public SettingsGroup
{
    Q_OBJECT
    
public:
    RTKSettings(QObject* parent = NULL);

    Q_PROPERTY(Fact* surveyInAccuracyLimit          READ surveyInAccuracyLimit          CONSTANT)
    Q_PROPERTY(Fact* surveyInMinObservationDuration READ surveyInMinObservationDuration CONSTANT)
    Q_PROPERTY(Fact* useFixedBasePosition           READ useFixedBasePosition           CONSTANT)
    Q_PROPERTY(Fact* fixedBasePositionLatitude      READ fixedBasePositionLatitude      CONSTANT)
    Q_PROPERTY(Fact* fixedBasePositionLongitude     READ fixedBasePositionLongitude     CONSTANT)
    Q_PROPERTY(Fact* fixedBasePositionAltitude      READ fixedBasePositionAltitude      CONSTANT)
    Q_PROPERTY(Fact* fixedBasePositionAccuracy      READ fixedBasePositionAccuracy      CONSTANT)

    Fact* surveyInAccuracyLimit         (void);
    Fact* surveyInMinObservationDuration(void);
    Fact* useFixedBasePosition          (void);
    Fact* fixedBasePositionLatitude     (void);
    Fact* fixedBasePositionLongitude    (void);
    Fact* fixedBasePositionAltitude     (void);
    Fact* fixedBasePositionAccuracy     (void);

    static const char* name;
    static const char* settingsGroup;

    static const char* surveyInAccuracyLimitName;
    static const char* surveyInMinObservationDurationName;
    static const char* useFixedBasePositionName;
    static const char* fixedBasePositionLatitudeName;
    static const char* fixedBasePositionLongitudeName;
    static const char* fixedBasePositionAltitudeName;
    static const char* fixedBasePositionAccuracyName;

private:
    SettingsFact* _surveyInAccuracyLimitFact;
    SettingsFact* _surveyInMinObservationDurationFact;
    SettingsFact* _useFixedBasePositionFact;
    SettingsFact* _fixedBasePositionLatitudeFact;
    SettingsFact* _fixedBasePositionLongitudeFact;
    SettingsFact* _fixedBasePositionAltitudeFact;
    SettingsFact* _fixedBasePositionAccuracyFact;
};
