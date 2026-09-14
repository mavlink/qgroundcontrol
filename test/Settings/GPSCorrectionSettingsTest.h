#pragma once

#include "UnitTest.h"

class GPSCorrectionSettingsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _storageNamespace_data();
    void _storageNamespace();
    void _metadataPartition();
    void _routingDefaults();
    void _qmlRegistration();
    void _routingPanel_data();
    void _routingPanel();
    void _routingPanelTracksStreams();
};
