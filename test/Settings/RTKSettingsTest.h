#pragma once

#include <QtCore/QHash>
#include <QtCore/QString>
#include <QtCore/QVariant>

#include "UnitTest.h"

/// Migrations of earlier receiver settings, including those once kept in the AutoConnect group.
class RTKSettingsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init() override;
    void cleanup() override;

    void _autoConnectMigration();
    void _nmeaInputBecomesPassiveReceiver_data();
    void _nmeaInputBecomesPassiveReceiver();
    void _nmeaPortLabelBecomesPassiveReceiver_data();
    void _nmeaPortLabelBecomesPassiveReceiver();
    void _configuredReceiverKeepsSettings();
    void _manufacturerMigration_data();
    void _manufacturerMigration();
    void _positionOnlyRoleBecomesPassive();

private:
    QHash<QString, QHash<QString, QVariant>> _savedGroups;
};
