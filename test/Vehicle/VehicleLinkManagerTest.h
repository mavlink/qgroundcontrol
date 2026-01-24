#pragma once

#include "TestFixtures.h"
#include "LinkInterface.h"
#include "LinkConfiguration.h"

/// Tests for VehicleLinkManager functionality.
/// Uses UnitTest directly since it manages multiple links with custom setup/teardown.
class VehicleLinkManagerTest : public UnitTest
{
    Q_OBJECT

public:
    VehicleLinkManagerTest() = default;

protected:
    void init() final;
    void cleanup() final;

private slots:
    void _simpleLinkTest();
    void _simpleCommLossTest();
    void _multiLinkSingleVehicleTest();
    void _connectionRemovedTest();
    void _highLatencyLinkTest();

private:
    void _startMockLink(int mockIndex, bool highLatency, bool incrementVehicleId, SharedLinkConfigurationPtr &sharedConfig, SharedLinkInterfacePtr &mockLink);

    static constexpr const char *_primaryLinkChangedSignalName = "primaryLinkChanged";
    static constexpr const char *_allLinksRemovedSignalName = "allLinksRemoved";
    static constexpr const char *_communicationLostChangedSignalName = "communicationLostChanged";
    static constexpr const char *_communicationLostEnabledChangedSignalName = "communicationLostEnabledChanged";
    static constexpr const char *_linkNamesChangedSignalName = "linkNamesChanged";
    static constexpr const char *_linkStatusesChangedSignalName = "linkStatusesChanged";
};
