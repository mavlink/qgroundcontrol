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
    void _nmeaInputBecomesPositionOnlyReceiver_data();
    void _nmeaInputBecomesPositionOnlyReceiver();
    void _nmeaPortLabelBecomesPositionOnlyReceiver_data();
    void _nmeaPortLabelBecomesPositionOnlyReceiver();
    void _configuredReceiverKeepsSettings();
    void _passiveManufacturerBecomesRole();

private:
    QHash<QString, QHash<QString, QVariant>> _savedGroups;
};
