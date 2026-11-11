#pragma once

#include "UnitTest.h"

/// Migrations of earlier receiver settings, including those once kept in the AutoConnect group, and the saved base
/// sites.
class RTKSettingsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _autoConnectMigration();
    void _nmeaInputBecomesPassiveReceiver_data();
    void _nmeaInputBecomesPassiveReceiver();
    void _configuredReceiverKeepsSettings();
    void _renamedSettingsMigration_data();
    void _renamedSettingsMigration();
    void _manufacturerMigration_data();
    void _manufacturerMigration();
    void _baseSitesLoad_data();
    void _baseSitesLoad();
    void _baseSitesSaveAndRemove();
    void _baseSiteBecomesFixedPosition();
};
