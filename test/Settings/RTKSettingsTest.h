#pragma once

#include "UnitTest.h"

/// Migrations of earlier receiver settings, including those once kept in the AutoConnect group.
class RTKSettingsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _autoConnectMigration();
    void _nmeaInputBecomesPassiveReceiver_data();
    void _nmeaInputBecomesPassiveReceiver();
    void _configuredReceiverKeepsSettings();
    void _manufacturerMigration_data();
    void _manufacturerMigration();
};
