#pragma once

#include "UnitTest.h"

/// The settings bindings: the receiver's configuration from its settings, the NTRIP configuration and its connection
/// setting, redacted logging and that every setting is bound.
class GPSSettingsBindingsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _receiverSettingsMapping_data();
    void _receiverSettingsMapping();
    void _receiverSettingsBinding();
    void _positionReceiverSettingsBinding();
    void _runtimeSettingsDoNotRequireAppRestart();
    void _ntripSettingsBinding();
    void _ntripActionsFollowConnectionSetting();
    void _settingsLogRedactsSecrets_data();
    void _settingsLogRedactsSecrets();
    void _settingsBindingsCoverEveryFact_data();
    void _settingsBindingsCoverEveryFact();
};
