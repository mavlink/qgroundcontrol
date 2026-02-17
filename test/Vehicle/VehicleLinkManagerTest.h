#pragma once

#include "BaseClasses/CommsTest.h"
#include "LinkConfiguration.h"
#include "LinkInterface.h"
#include "MockLink.h"

class VehicleLinkManagerTest : public CommsTest
{
    Q_OBJECT

private slots:
    void _simpleLinkTest();
    void _simpleCommLossTest();
    void _multiLinkSingleVehicleTest();
    void _connectionRemovedTest();
    void _highLatencyLinkTest();

private:
    void _startMockLink(int mockIndex, bool highLatency, bool incrementVehicleId,
                        SharedLinkConfigurationPtr& sharedConfig, SharedLinkInterfacePtr& mockLink);

    static constexpr const char* _primaryLinkChangedSignalName = "primaryLinkChanged";
    static constexpr const char* _allLinksRemovedSignalName = "allLinksRemoved";
    static constexpr const char* _communicationLostChangedSignalName = "communicationLostChanged";
    static constexpr const char* _communicationLostEnabledChangedSignalName = "communicationLostEnabledChanged";
    static constexpr const char* _linkNamesChangedSignalName = "linkNamesChanged";
    static constexpr const char* _linkStatusesChangedSignalName = "linkStatusesChanged";
};
