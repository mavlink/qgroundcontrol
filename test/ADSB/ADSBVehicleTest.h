#pragma once

#include "UnitTest.h"
#include "ADSB.h"

#include <QtPositioning/QGeoCoordinate>

/// Unit tests for ADSBVehicle class.
/// Pure unit tests that don't require singletons or network.
class ADSBVehicleTest : public UnitTest
{
    Q_OBJECT

public:
    ADSBVehicleTest() = default;

private slots:
    // Override to skip UnitTest's heavyweight initialization
    // This is a pure unit test that doesn't need MultiVehicleManager or LinkManager
    void init() override {}
    void cleanup() override {}

    void _constructorTest();
    void _updateSameIcaoTest();
    void _updateDifferentIcaoIgnoredTest();
    void _expirationTest();
    void _availableFlagsTest();

private:
    static ADSB::VehicleInfo_t _createVehicleInfo(
        uint32_t icaoAddress,
        const QString &callsign = QString(),
        const QGeoCoordinate &location = QGeoCoordinate(),
        ADSB::AvailableInfoTypes flags = ADSB::CallsignAvailable);
};
