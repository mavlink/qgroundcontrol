#pragma once

#include <QtCore/QHash>
#include <QtCore/QString>
#include <QtCore/QVariant>

#include "UnitTest.h"

/// Settings migrations for the ground-station position source and the unified GNSS receiver input.
class AutoConnectSettingsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init() override;
    void cleanup() override;

    void _nmeaPositionSourceMigration();
    void _nmeaInputBecomesPositionOnlyReceiver_data();
    void _nmeaInputBecomesPositionOnlyReceiver();
    void _nmeaPortLabelBecomesPositionOnlyReceiver_data();
    void _nmeaPortLabelBecomesPositionOnlyReceiver();
    void _configuredReceiverKeepsSettings();
    void _passiveManufacturerBecomesRole();

private:
    QHash<QString, QHash<QString, QVariant>> _savedGroups;
};
