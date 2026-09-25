#pragma once

#include "UnitTest.h"

class GPSCorrectionManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _sourcesShareForwarder();
    void _mavlinkDestinationAdmissions();
    void _outputsEnabledAfterLinkHistoryChurn();
    void _udpOutputForwardsSelectedStream();
    void _udpOutputSkipsOwnInput();
    void _udpOutputEndpointChanges_data();
    void _udpOutputEndpointChanges();
    void _sourceTopologyDoesNotNotifyOnCounters();
    void _diagnosticsNotifyOnlyOnChange();
    void _healthSampleObserverCanShutDown();
    void _sourceMessageCounts();
    void _diagnosticsModelUpdatesInPlace();
    void _udpSettingsAndShutdown();
    void _settingsOwnRouting_data();
    void _settingsOwnRouting();
    void _shutdownDuringAdmission_data();
    void _shutdownDuringAdmission();
    void _qmlForwarderAvailableBeforeInit();
    void _correctionsStatusShowsSelectedStream();
};
