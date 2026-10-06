#pragma once

#include "UnitTest.h"

class MavlinkSettingsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _noInitialDownloadWhenFlyingMigration_data();
    void _noInitialDownloadWhenFlyingMigration();
};
