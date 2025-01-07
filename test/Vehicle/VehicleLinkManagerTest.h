/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "UnitTest.h"
#include "LinkInterface.h"
#include "LinkConfiguration.h"

class VehicleLinkManagerTest : public UnitTest
{
    Q_OBJECT

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
