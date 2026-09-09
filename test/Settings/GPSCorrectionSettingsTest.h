#pragma once

#include "UnitTest.h"

class GPSCorrectionSettingsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _legacyKeysAndFactIdentity_data();
    void _legacyKeysAndFactIdentity();
    void _metadataPartition();
    void _qmlRegistration();
};
