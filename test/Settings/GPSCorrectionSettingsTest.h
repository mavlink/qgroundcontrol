#pragma once

#include "UnitTest.h"

class GPSCorrectionSettingsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _storageNamespace_data();
    void _storageNamespace();
    void _metadataPartition();
    void _udpOutputKeysMigrate();
    void _removedSettingsAreDropped_data();
    void _removedSettingsAreDropped();
    void _routingDefaults();
    void _qmlRegistration();
};
