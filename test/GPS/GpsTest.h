#pragma once

#include "TestFixtures.h"

/// Unit tests for GPS functionality.
/// Tests RTCMMavlink, GPSProvider types, sensor structures, RTCMParser, and GPSRTKFactGroup.
/// Uses OfflineTest since it doesn't require a vehicle connection.
class GpsTest : public OfflineTest
{
    Q_OBJECT

public:
    GpsTest() = default;

private slots:
    // RTCMMavlink tests
    void _testRTCMMavlinkCreation();
    void _testRTCMMavlinkParent();

    // GPSProvider type tests
    void _testGPSProviderTypeEnumUBlox();
    void _testGPSProviderTypeEnumTrimble();
    void _testGPSProviderTypeEnumSeptentrio();
    void _testGPSProviderTypeEnumFemto();
    void _testGPSProviderTypeEnumDistinct();

    // RTK data struct tests
    void _testRTKDataStructDefaults();
    void _testRTKDataStructSurveyInAccuracy();
    void _testRTKDataStructDuration();
    void _testRTKDataStructFixedBaseMode();
    void _testRTKDataStructCoordinates();

    // BaseModeDefinition tests
    void _testBaseModeDefinitionSurveyIn();
    void _testBaseModeDefinitionFixed();
    void _testBaseModeDefinitionDistinct();

    // sensor_gps_s constant tests
    void _testSensorGpsFixTypeNone();
    void _testSensorGpsFixType2D();
    void _testSensorGpsFixType3D();
    void _testSensorGpsFixTypeRTCMCodeDiff();
    void _testSensorGpsFixTypeRTKFloat();
    void _testSensorGpsFixTypeRTKFixed();
    void _testSensorGpsFixTypeExtrapolated();
    void _testSensorGpsJammingStateUnknown();
    void _testSensorGpsJammingStateOK();
    void _testSensorGpsJammingStateWarning();
    void _testSensorGpsJammingStateCritical();
    void _testSensorGpsSpoofingStateUnknown();
    void _testSensorGpsSpoofingStateNone();
    void _testSensorGpsSpoofingStateIndicated();
    void _testSensorGpsSpoofingStateMultiple();
    void _testSensorGpsRTCMMsgUsedUnknown();
    void _testSensorGpsRTCMMsgUsedNotUsed();
    void _testSensorGpsRTCMMsgUsedUsed();
    void _testSensorGpsStructDefaults();

    // satellite_info_s tests
    void _testSatelliteInfoMaxSatellites();
    void _testSatelliteInfoArraySizes();
    void _testSatelliteInfoDefaults();

    // sensor_gnss_relative_s tests
    void _testSensorGnssRelativeDefaults();
    void _testSensorGnssRelativeBooleanFields();

    // RTCMParser tests
    void _testRTCMParserCreation();
    void _testRTCMParserReset();
    void _testRTCMParserPreambleConstant();
    void _testRTCMParserCrcSize();
    void _testRTCMParserInitialState();
    void _testRTCMParserAddPreambleByte();

    // GPSRTKFactGroup tests
    void _testGPSRTKFactGroupCreation();
    void _testGPSRTKFactGroupConnectedFact();
    void _testGPSRTKFactGroupDurationFact();
    void _testGPSRTKFactGroupAccuracyFact();
    void _testGPSRTKFactGroupLatitudeFact();
    void _testGPSRTKFactGroupLongitudeFact();
    void _testGPSRTKFactGroupAltitudeFact();
    void _testGPSRTKFactGroupValidFact();
    void _testGPSRTKFactGroupActiveFact();
    void _testGPSRTKFactGroupNumSatellitesFact();

    // Conversion constant tests
    void _testDegToRadConversion();
    void _testRadToDegConversion();
    void _testDegToRadFloatConversion();
    void _testRadToDegFloatConversion();
};
