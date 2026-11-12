#pragma once

#include "UnitTest.h"

/// A receiver session's base-station settings: the receiver request and base state they produce.
class GPSReceiverSessionTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _configForSettings_data();
    void _configForSettings();
    void _settingsDiagnostics_data();
    void _settingsDiagnostics();
    void _compactObservationsFollowSupport();
    void _correctionConstellationsFollowSupport();
    void _persistentConsent_data();
    void _persistentConsent();
    void _baseStationState();
    void _baseStatusUpdatesBasePosition_data();
    void _baseStatusUpdatesBasePosition();
};
