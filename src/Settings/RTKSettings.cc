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

const char* RTKSettings::name =                                 "RTK";
const char* RTKSettings::settingsGroup =                        "RTK";

const char* RTKSettings::surveyInAccuracyLimitName =            "SurveyInAccuracyLimit";
const char* RTKSettings::surveyInMinObservationDurationName =   "SurveyInMinObservationDuration";
const char* RTKSettings::useFixedBasePositionName =             "UseFixedBasePosition";
const char* RTKSettings::fixedBasePositionLatitudeName =        "FixedBasePositionLatitude";
const char* RTKSettings::fixedBasePositionLongitudeName =       "FixedBasePositionLongitude";
const char* RTKSettings::fixedBasePositionAltitudeName =        "FixedBasePositionAltitude";
const char* RTKSettings::fixedBasePositionAccuracyName =        "FixedBasePositionAccuracy";

RTKSettings::RTKSettings(QObject* parent)
    : SettingsGroup                         (name, settingsGroup, parent)
    , _surveyInAccuracyLimitFact            (nullptr)
    , _surveyInMinObservationDurationFact   (nullptr)
    , _useFixedBasePositionFact             (nullptr)
    , _fixedBasePositionLatitudeFact        (nullptr)
    , _fixedBasePositionLongitudeFact       (nullptr)
    , _fixedBasePositionAltitudeFact        (nullptr)
    , _fixedBasePositionAccuracyFact        (nullptr)
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

Fact* RTKSettings::useFixedBasePosition(void)
{
    if (!_useFixedBasePositionFact) {
        _useFixedBasePositionFact = _createSettingsFact(useFixedBasePositionName);
    }

    return _useFixedBasePositionFact;
}

Fact* RTKSettings::fixedBasePositionLatitude(void)
{
    if (!_fixedBasePositionLatitudeFact) {
        _fixedBasePositionLatitudeFact = _createSettingsFact(fixedBasePositionLatitudeName);
    }

    return _fixedBasePositionLatitudeFact;
}

Fact* RTKSettings::fixedBasePositionLongitude(void)
{
    if (!_fixedBasePositionLongitudeFact) {
        _fixedBasePositionLongitudeFact = _createSettingsFact(fixedBasePositionLongitudeName);
    }

    return _fixedBasePositionLongitudeFact;
}

Fact* RTKSettings::fixedBasePositionAltitude(void)
{
    if (!_fixedBasePositionAltitudeFact) {
        _fixedBasePositionAltitudeFact = _createSettingsFact(fixedBasePositionAltitudeName);
    }

    return _fixedBasePositionAltitudeFact;
}

Fact* RTKSettings::fixedBasePositionAccuracy(void)
{
    if (!_fixedBasePositionAccuracyFact) {
        _fixedBasePositionAccuracyFact = _createSettingsFact(fixedBasePositionAccuracyName);
    }

    return _fixedBasePositionAccuracyFact;
}
