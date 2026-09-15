#pragma once

#include "UnitTest.h"

class GPSCorrectionManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _sourcesShareForwarder();
    void _mavlinkDestinationAdmissions();
    void _outputsEnabledAfterLinkHistoryChurn();
    void _ntripUdpOutputIsSourceSpecific();
    void _ntripUdpOutputEndpointChanges_data();
    void _ntripUdpOutputEndpointChanges();
    void _sourceTopologyDoesNotNotifyOnCounters();
    void _sourceSelectionAndSessions();
    void _filteredAndExpiredFrames();
    void _udpSettingsAndShutdown();
    void _settingsOwnRouting_data();
    void _settingsOwnRouting();
    void _shutdownDuringDelivery_data();
    void _shutdownDuringDelivery();
    void _shutdownDuringAdmission_data();
    void _shutdownDuringAdmission();
    void _qmlForwarderAvailableBeforeInit();
};
