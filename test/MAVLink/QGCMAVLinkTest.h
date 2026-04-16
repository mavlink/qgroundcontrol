#pragma once

#include "UnitTest.h"

/// Unit tests for the pure static helpers in QGCMAVLink.
///
/// QGCMAVLink is a large static utility namespace that exposes classification
/// helpers (isFixedWing, isMultiRotor, vehicleClass, firmwareClass, ...),
/// stringification (mavResultToString, mavTypeToString, compIdToString, ...),
/// and small data tables (allFirmwareClasses, motorCount). These helpers are
/// dependency-free and therefore ideal for fast unit coverage — they
/// previously had no direct test.
class QGCMAVLinkTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testFirmwareClassClassification();
    void _testFirmwareClassRoundTrip();
    void _testFirmwareClassToStringUnknown();
    void _testFirmwareTypeFromStringWhitespace();

    void _testVehicleClassClassification();
    void _testVehicleClassVTOL();
    void _testVehicleClassRoverBoatCovers();
    void _testVehicleClassGenericFallback();
    void _testVehicleClassToString();
    void _testVehicleClassInternalString();
    void _testVehicleTypeFromString();

    void _testMotorCountStandardTypes();
    void _testMotorCountSubmarineFrames();
    void _testMotorCountUnknownReturnsNegative();

    void _testMavResultToString();
    void _testMavResultToStringUnknown();

    void _testFirmwareVersionTypeToString();
    void _testFirmwareVersionTypeRoundTrip();

    void _testCompIdToStringKnown();
    void _testCompIdToStringUnknown();

    void _testIsValidChannel();

    void _testAllFirmwareClasses();
    void _testAllVehicleClasses();
};
