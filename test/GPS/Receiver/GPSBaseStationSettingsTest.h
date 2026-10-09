#pragma once

#include "UnitTest.h"

/// Saved base-station settings: the receiver request and base state they produce.
class GPSBaseStationSettingsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _configForSettings_data();
    void _configForSettings();
    void _settingsDiagnostics_data();
    void _settingsDiagnostics();
    void _compactObservationsFollowSupport();
    void _persistentConsent_data();
    void _persistentConsent();
    void _baseStationState();
    void _surveyUpdatesBasePosition_data();
    void _surveyUpdatesBasePosition();
};
