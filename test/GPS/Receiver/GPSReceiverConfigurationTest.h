#pragma once

#include "UnitTest.h"

/// Receiver configuration: settings mapping, roles, manufacturers, consent and the effective connection.
class GPSReceiverConfigurationTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _manufacturerIds_data();
    void _manufacturerIds();
    void _receiverSettingsMapping_data();
    void _receiverSettingsMapping();
    void _invalidReceiverSettings_data();
    void _invalidReceiverSettings();
    void _configurationDiagnosticRetained_data();
    void _configurationDiagnosticRetained();
    void _rtkSettingsBinding();
    void _persistentConsentScope();
    void _persistentConsentIsSpentOnConnect_data();
    void _persistentConsentIsSpentOnConnect();
    void _effectiveConnection_data();
    void _effectiveConnection();
    void _configurationDebugRedactsFixedBaseCoordinates();
    void _runtimeSettingsDoNotRequireAppRestart_data();
    void _runtimeSettingsDoNotRequireAppRestart();
};
