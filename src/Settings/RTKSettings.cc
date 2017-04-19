/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "RTKSettings.h"

#include <QQmlEngine>
#include <QtQml>

const char* RTKSettings::RTKSettingsGroupName =                 "RTK";
const char* RTKSettings::surveyInAccuracyLimitName =            "SurveyInAccuracyLimit";
const char* RTKSettings::surveyInMinObservationDurationName =   "SurveyInMinObservationDuration";

RTKSettings::RTKSettings(QObject* parent)
    : SettingsGroup(RTKSettingsGroupName, QString(RTKSettingsGroupName), parent)
    , _surveyInAccuracyLimitFact(NULL)
    , _surveyInMinObservationDurationFact(NULL)
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    qmlRegisterUncreatableType<RTKSettings>("QGroundControl.SettingsManager", 1, 0, "RTKSettings", "Reference only");
}

Fact* RTKSettings::surveyInAccuracyLimit(void)
{
    if (!_surveyInAccuracyLimitFact) {
        _surveyInAccuracyLimitFact = _createSettingsFact(surveyInAccuracyLimitName);
    }

    return _surveyInAccuracyLimitFact;
}

Fact* RTKSettings::surveyInMinObservationDuration(void)
{
    if (!_surveyInMinObservationDurationFact) {
        _surveyInMinObservationDurationFact = _createSettingsFact(surveyInMinObservationDurationName);
    }

    return _surveyInMinObservationDurationFact;
}
