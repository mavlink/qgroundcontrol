#pragma once

#include "UnitTest.h"

class GPSCorrectionManager;
class GPSCorrectionSettings;

class GPSCorrectionManagerTest : public UnitTest
{
    Q_OBJECT

    /// Configures the bound UDP input with a free port and returns it once listening, or 0. Retries a port another
    /// process takes before the input binds it.
    quint16 _listenOnUnusedPort(GPSCorrectionSettings* settings, const GPSCorrectionManager& corrections);

private slots:
    void _sourcesShareForwarder();
    void _udpOutputForwardsSelectedStream();
    void _udpOutputSkipsOwnInput();
    void _udpOutputSkipsHostTargets_data();
    void _udpOutputSkipsHostTargets();
    void _udpOutputEndpointChanges_data();
    void _udpOutputEndpointChanges();
    void _sourceTopologyDoesNotNotifyOnCounters();
    void _udpSettingsAndShutdown();
    void _udpInputRetriesUnavailablePort();
    void _settingsOwnRouting_data();
    void _settingsOwnRouting();
};
