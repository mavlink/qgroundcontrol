#pragma once

#include "UnitTest.h"

class GPSCorrectionManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _sourcesShareForwarder();
    void _udpSettingsAndShutdown();
    void _shutdownDuringDelivery_data();
    void _shutdownDuringDelivery();
    void _qmlForwarderAvailableBeforeInit();
};
