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

class LinkManager;
class MultiVehicleManager;
class VehicleLinkManager;

class VehicleLinkManagerTest : public UnitTest
{
    Q_OBJECT

public:
    VehicleLinkManagerTest(void);

protected:
    void init   (void) final;
    void cleanup(void) final;

private slots:
    void _simpleLinkTest            (void);
    void _simpleCommLossTest        (void);
    void _multiLinkSingleVehicleTest(void);
    void _connectionRemovedTest     (void);
    void _highLatencyLinkTest       (void);

private:
    void _startMockLink(int mockIndex, bool highLatency, bool incrementVehicleId, SharedLinkConfigurationPtr& sharedConfig, SharedLinkInterfacePtr& mockLink);

    //void _simpleLinkTest            (void);
    //void _simpleCommLossTest        (void);
    //void _multiLinkSingleVehicleTest(void);
    //void _connectionRemovedTest     (void);
    //void _highLatencyLinkTest       (void);
    MultiVehicleManager*    _multiVehicleMgr = nullptr;

    static const char* _primaryLinkChangedSignalName;
    static const char* _allLinksRemovedSignalName;
    static const char* _communicationLostChangedSignalName;
    static const char* _communicationLostEnabledChangedSignalName;
    static const char* _linkNamesChangedSignalName;
    static const char* _linkStatusesChangedSignalName;
};
