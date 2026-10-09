#pragma once

#include "UnitTest.h"

/// The settings bindings: the receiver's configuration from its settings (mapping, roles, manufacturer capabilities,
/// consent and the effective connection), the NTRIP configuration, redacted logging and that every setting is bound.
class GPSReceiverSettingsBindingTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _capabilitiesFor_data();
    void _capabilitiesFor();
    void _receiverSettingsMapping_data();
    void _receiverSettingsMapping();
    void _configurationDiagnosticRetained_data();
    void _configurationDiagnosticRetained();
    void _receiverSettingsBinding();
    void _persistentConsentScope_data();
    void _persistentConsentScope();
    void _persistentConsentIsSpentOnConnect_data();
    void _persistentConsentIsSpentOnConnect();
    void _effectiveConnection_data();
    void _effectiveConnection();
    void _runtimeSettingsDoNotRequireAppRestart();
    void _ntripSettingsBinding();
    void _settingsLogRedactsSecrets_data();
    void _settingsLogRedactsSecrets();
    void _settingsBindingsCoverEveryFact_data();
    void _settingsBindingsCoverEveryFact();
};
