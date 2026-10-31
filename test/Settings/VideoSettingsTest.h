#pragma once

#include "UnitTest.h"

class VideoSettingsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _streamEnabledMigration_data();
    void _streamEnabledMigration();
};
