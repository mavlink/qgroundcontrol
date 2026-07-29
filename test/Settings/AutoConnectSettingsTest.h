#pragma once

#include <QtCore/QVariant>

#include "UnitTest.h"

class AutoConnectSettingsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init() override;
    void cleanup() override;

    void _legacyNmeaPortMigration_data();
    void _legacyNmeaPortMigration();
    void _nmeaMigrationSkippedWhenSourceAlreadySet();

private:
    QVariant _savedNmeaSource;
    QVariant _savedNmeaPort;
    bool _hadNmeaSource = false;
    bool _hadNmeaPort = false;
};
