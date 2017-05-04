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

    Fact* surveyInAccuracyLimit         (void);
    Fact* surveyInMinObservationDuration(void);

    static const char* RTKSettingsGroupName;

    static const char* surveyInAccuracyLimitName;
    static const char* surveyInMinObservationDurationName;

private:
    SettingsFact* _surveyInAccuracyLimitFact;
    SettingsFact* _surveyInMinObservationDurationFact;
};
