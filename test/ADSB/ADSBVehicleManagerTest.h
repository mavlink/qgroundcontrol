#pragma once

#include "UnitTest.h"
#include "ADSB.h"

#include <QtPositioning/QGeoCoordinate>

/// Unit tests for ADSBVehicleManager singleton.
/// Requires GlobalState resource lock since it uses the singleton instance.
class ADSBVehicleManagerTest : public UnitTest
{
    Q_OBJECT

public:
    ADSBVehicleManagerTest() = default;

private slots:
    void _instanceTest();
    void _addVehicleTest();
    void _updateExistingVehicleTest();
    void _mavlinkMessageTest();

private:
    static ADSB::VehicleInfo_t _createVehicleInfo(
        uint32_t icaoAddress,
        const QString &callsign = QString(),
        const QGeoCoordinate &location = QGeoCoordinate(),
        ADSB::AvailableInfoTypes flags = ADSB::CallsignAvailable | ADSB::LocationAvailable);
};
