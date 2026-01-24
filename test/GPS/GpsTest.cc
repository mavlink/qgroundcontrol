#include "GpsTest.h"
#include "GPSProvider.h"
#include "GPSRTKFactGroup.h"
#include "NTRIP.h"
#include "RTCMMavlink.h"
#include "definitions.h"
#include "satellite_info.h"
#include "sensor_gnss_relative.h"
#include "sensor_gps.h"

#include <QtCore/QtMath>
#include <QtTest/QTest>

// ============================================================================
// RTCMMavlink Tests
// ============================================================================

void GpsTest::_testRTCMMavlinkCreation()
{
    RTCMMavlink* rtcm = new RTCMMavlink(this);
    VERIFY_NOT_NULL(rtcm);
    delete rtcm;
}

void GpsTest::_testRTCMMavlinkParent()
{
    RTCMMavlink* rtcm = new RTCMMavlink(this);
    QCOMPARE(rtcm->parent(), this);
    delete rtcm;
}

// ============================================================================
// GPSProvider Type Tests
// ============================================================================

void GpsTest::_testGPSProviderTypeEnumUBlox()
{
    GPSProvider::GPSType type = GPSProvider::GPSType::u_blox;
    QCOMPARE(type, GPSProvider::GPSType::u_blox);
}

void GpsTest::_testGPSProviderTypeEnumTrimble()
{
    GPSProvider::GPSType type = GPSProvider::GPSType::trimble;
    QCOMPARE(type, GPSProvider::GPSType::trimble);
}

void GpsTest::_testGPSProviderTypeEnumSeptentrio()
{
    GPSProvider::GPSType type = GPSProvider::GPSType::septentrio;
    QCOMPARE(type, GPSProvider::GPSType::septentrio);
}

void GpsTest::_testGPSProviderTypeEnumFemto()
{
    GPSProvider::GPSType type = GPSProvider::GPSType::femto;
    QCOMPARE(type, GPSProvider::GPSType::femto);
}

void GpsTest::_testGPSProviderTypeEnumDistinct()
{
    QVERIFY(GPSProvider::GPSType::u_blox != GPSProvider::GPSType::trimble);
    QVERIFY(GPSProvider::GPSType::trimble != GPSProvider::GPSType::septentrio);
    QVERIFY(GPSProvider::GPSType::septentrio != GPSProvider::GPSType::femto);
    QVERIFY(GPSProvider::GPSType::u_blox != GPSProvider::GPSType::femto);
    QVERIFY(GPSProvider::GPSType::u_blox != GPSProvider::GPSType::septentrio);
    QVERIFY(GPSProvider::GPSType::trimble != GPSProvider::GPSType::femto);
}

// ============================================================================
// RTK Data Struct Tests
// ============================================================================

void GpsTest::_testRTKDataStructDefaults()
{
    GPSProvider::rtk_data_s rtkData{};

    QCOMPARE_EQ(rtkData.surveyInAccMeters, 0.0);
    QCOMPARE_EQ(rtkData.surveyInDurationSecs, 0);
    QCOMPARE_EQ(rtkData.useFixedBaseLocation, BaseModeDefinition::Mode::BaseSurveyIn);
    QCOMPARE_EQ(rtkData.fixedBaseLatitude, 0.0);
    QCOMPARE_EQ(rtkData.fixedBaseLongitude, 0.0);
    QCOMPARE_EQ(rtkData.fixedBaseAltitudeMeters, 0.0f);
    QCOMPARE_EQ(rtkData.fixedBaseAccuracyMeters, 0.0f);
}

void GpsTest::_testRTKDataStructSurveyInAccuracy()
{
    GPSProvider::rtk_data_s rtkData{};
    rtkData.surveyInAccMeters = 2.5;
    QCOMPARE_EQ(rtkData.surveyInAccMeters, 2.5);

    rtkData.surveyInAccMeters = 0.001;
    QCOMPARE_EQ(rtkData.surveyInAccMeters, 0.001);
}

void GpsTest::_testRTKDataStructDuration()
{
    GPSProvider::rtk_data_s rtkData{};
    rtkData.surveyInDurationSecs = 60;
    QCOMPARE_EQ(rtkData.surveyInDurationSecs, 60);

    rtkData.surveyInDurationSecs = 3600;
    QCOMPARE_EQ(rtkData.surveyInDurationSecs, 3600);
}

void GpsTest::_testRTKDataStructFixedBaseMode()
{
    GPSProvider::rtk_data_s rtkData{};
    rtkData.useFixedBaseLocation = BaseModeDefinition::Mode::BaseFixed;
    QCOMPARE_EQ(rtkData.useFixedBaseLocation, BaseModeDefinition::Mode::BaseFixed);
}

void GpsTest::_testRTKDataStructCoordinates()
{
    GPSProvider::rtk_data_s rtkData{};

    rtkData.fixedBaseLatitude = 47.3977;
    rtkData.fixedBaseLongitude = 8.5456;
    rtkData.fixedBaseAltitudeMeters = 408.0f;
    rtkData.fixedBaseAccuracyMeters = 0.5f;

    QCOMPARE_EQ(rtkData.fixedBaseLatitude, 47.3977);
    QCOMPARE_EQ(rtkData.fixedBaseLongitude, 8.5456);
    QCOMPARE_EQ(rtkData.fixedBaseAltitudeMeters, 408.0f);
    QCOMPARE_EQ(rtkData.fixedBaseAccuracyMeters, 0.5f);
}

// ============================================================================
// BaseModeDefinition Tests
// ============================================================================

void GpsTest::_testBaseModeDefinitionSurveyIn()
{
    QCOMPARE_EQ(static_cast<int>(BaseModeDefinition::Mode::BaseSurveyIn), 0);
}

void GpsTest::_testBaseModeDefinitionFixed()
{
    QCOMPARE_EQ(static_cast<int>(BaseModeDefinition::Mode::BaseFixed), 1);
}

void GpsTest::_testBaseModeDefinitionDistinct()
{
    QVERIFY(BaseModeDefinition::Mode::BaseSurveyIn != BaseModeDefinition::Mode::BaseFixed);
}

// ============================================================================
// sensor_gps_s Constant Tests
// ============================================================================

void GpsTest::_testSensorGpsFixTypeNone()
{
    QCOMPARE_EQ(sensor_gps_s::FIX_TYPE_NONE, static_cast<uint8_t>(1));
}

void GpsTest::_testSensorGpsFixType2D()
{
    QCOMPARE_EQ(sensor_gps_s::FIX_TYPE_2D, static_cast<uint8_t>(2));
}

void GpsTest::_testSensorGpsFixType3D()
{
    QCOMPARE_EQ(sensor_gps_s::FIX_TYPE_3D, static_cast<uint8_t>(3));
}

void GpsTest::_testSensorGpsFixTypeRTCMCodeDiff()
{
    QCOMPARE_EQ(sensor_gps_s::FIX_TYPE_RTCM_CODE_DIFFERENTIAL, static_cast<uint8_t>(4));
}

void GpsTest::_testSensorGpsFixTypeRTKFloat()
{
    QCOMPARE_EQ(sensor_gps_s::FIX_TYPE_RTK_FLOAT, static_cast<uint8_t>(5));
}

void GpsTest::_testSensorGpsFixTypeRTKFixed()
{
    QCOMPARE_EQ(sensor_gps_s::FIX_TYPE_RTK_FIXED, static_cast<uint8_t>(6));
}

void GpsTest::_testSensorGpsFixTypeExtrapolated()
{
    QCOMPARE_EQ(sensor_gps_s::FIX_TYPE_EXTRAPOLATED, static_cast<uint8_t>(8));
}

void GpsTest::_testSensorGpsJammingStateUnknown()
{
    QCOMPARE_EQ(sensor_gps_s::JAMMING_STATE_UNKNOWN, static_cast<uint8_t>(0));
}

void GpsTest::_testSensorGpsJammingStateOK()
{
    QCOMPARE_EQ(sensor_gps_s::JAMMING_STATE_OK, static_cast<uint8_t>(1));
}

void GpsTest::_testSensorGpsJammingStateWarning()
{
    QCOMPARE_EQ(sensor_gps_s::JAMMING_STATE_WARNING, static_cast<uint8_t>(2));
}

void GpsTest::_testSensorGpsJammingStateCritical()
{
    QCOMPARE_EQ(sensor_gps_s::JAMMING_STATE_CRITICAL, static_cast<uint8_t>(3));
}

void GpsTest::_testSensorGpsSpoofingStateUnknown()
{
    QCOMPARE_EQ(sensor_gps_s::SPOOFING_STATE_UNKNOWN, static_cast<uint8_t>(0));
}

void GpsTest::_testSensorGpsSpoofingStateNone()
{
    QCOMPARE_EQ(sensor_gps_s::SPOOFING_STATE_NONE, static_cast<uint8_t>(1));
}

void GpsTest::_testSensorGpsSpoofingStateIndicated()
{
    QCOMPARE_EQ(sensor_gps_s::SPOOFING_STATE_INDICATED, static_cast<uint8_t>(2));
}

void GpsTest::_testSensorGpsSpoofingStateMultiple()
{
    QCOMPARE_EQ(sensor_gps_s::SPOOFING_STATE_MULTIPLE, static_cast<uint8_t>(3));
}

void GpsTest::_testSensorGpsRTCMMsgUsedUnknown()
{
    QCOMPARE_EQ(sensor_gps_s::RTCM_MSG_USED_UNKNOWN, static_cast<uint8_t>(0));
}

void GpsTest::_testSensorGpsRTCMMsgUsedNotUsed()
{
    QCOMPARE_EQ(sensor_gps_s::RTCM_MSG_USED_NOT_USED, static_cast<uint8_t>(1));
}

void GpsTest::_testSensorGpsRTCMMsgUsedUsed()
{
    QCOMPARE_EQ(sensor_gps_s::RTCM_MSG_USED_USED, static_cast<uint8_t>(2));
}

void GpsTest::_testSensorGpsStructDefaults()
{
    sensor_gps_s gps{};

    QCOMPARE_EQ(gps.timestamp, static_cast<uint64_t>(0));
    QCOMPARE_EQ(gps.device_id, static_cast<uint32_t>(0));
    QCOMPARE_EQ(gps.latitude_deg, 0.0);
    QCOMPARE_EQ(gps.longitude_deg, 0.0);
    QCOMPARE_EQ(gps.altitude_msl_m, 0.0);
    QCOMPARE_EQ(gps.fix_type, static_cast<uint8_t>(0));
    QCOMPARE_EQ(gps.satellites_used, static_cast<uint8_t>(0));
    QVERIFY(!gps.vel_ned_valid);
    QVERIFY(!gps.rtcm_crc_failed);
}

// ============================================================================
// satellite_info_s Tests
// ============================================================================

void GpsTest::_testSatelliteInfoMaxSatellites()
{
    QCOMPARE_EQ(satellite_info_s::SAT_INFO_MAX_SATELLITES, static_cast<uint8_t>(20));
}

void GpsTest::_testSatelliteInfoArraySizes()
{
    satellite_info_s info{};

    QCOMPARE_EQ(sizeof(info.svid) / sizeof(info.svid[0]), static_cast<size_t>(20));
    QCOMPARE_EQ(sizeof(info.used) / sizeof(info.used[0]), static_cast<size_t>(20));
    QCOMPARE_EQ(sizeof(info.elevation) / sizeof(info.elevation[0]), static_cast<size_t>(20));
    QCOMPARE_EQ(sizeof(info.azimuth) / sizeof(info.azimuth[0]), static_cast<size_t>(20));
    QCOMPARE_EQ(sizeof(info.snr) / sizeof(info.snr[0]), static_cast<size_t>(20));
    QCOMPARE_EQ(sizeof(info.prn) / sizeof(info.prn[0]), static_cast<size_t>(20));
}

void GpsTest::_testSatelliteInfoDefaults()
{
    satellite_info_s info{};

    QCOMPARE_EQ(info.timestamp, static_cast<uint64_t>(0));
    QCOMPARE_EQ(info.count, static_cast<uint8_t>(0));

    for (int i = 0; i < 20; i++) {
        QCOMPARE_EQ(info.svid[i], static_cast<uint8_t>(0));
        QCOMPARE_EQ(info.used[i], static_cast<uint8_t>(0));
    }
}

// ============================================================================
// sensor_gnss_relative_s Tests
// ============================================================================

void GpsTest::_testSensorGnssRelativeDefaults()
{
    sensor_gnss_relative_s gnssRel{};

    QCOMPARE_EQ(gnssRel.timestamp, static_cast<uint64_t>(0));
    QCOMPARE_EQ(gnssRel.device_id, static_cast<uint32_t>(0));
    QCOMPARE_EQ(gnssRel.reference_station_id, static_cast<uint16_t>(0));
    QCOMPARE_EQ(gnssRel.heading, 0.0f);
    QCOMPARE_EQ(gnssRel.heading_accuracy, 0.0f);
    QCOMPARE_EQ(gnssRel.position_length, 0.0f);
    QCOMPARE_EQ(gnssRel.accuracy_length, 0.0f);
}

void GpsTest::_testSensorGnssRelativeBooleanFields()
{
    sensor_gnss_relative_s gnssRel{};

    QVERIFY(!gnssRel.gnss_fix_ok);
    QVERIFY(!gnssRel.differential_solution);
    QVERIFY(!gnssRel.relative_position_valid);
    QVERIFY(!gnssRel.carrier_solution_floating);
    QVERIFY(!gnssRel.carrier_solution_fixed);
    QVERIFY(!gnssRel.moving_base_mode);
    QVERIFY(!gnssRel.reference_position_miss);
    QVERIFY(!gnssRel.reference_observations_miss);
    QVERIFY(!gnssRel.heading_valid);
    QVERIFY(!gnssRel.relative_position_normalized);

    gnssRel.gnss_fix_ok = true;
    gnssRel.carrier_solution_fixed = true;
    gnssRel.heading_valid = true;

    QVERIFY(gnssRel.gnss_fix_ok);
    QVERIFY(gnssRel.carrier_solution_fixed);
    QVERIFY(gnssRel.heading_valid);
}

// ============================================================================
// RTCMParser Tests
// ============================================================================

void GpsTest::_testRTCMParserCreation()
{
    RTCMParser parser;
    VERIFY_NOT_NULL(&parser);
}

void GpsTest::_testRTCMParserReset()
{
    RTCMParser parser;
    parser.reset();
    QCOMPARE_EQ(parser.messageLength(), static_cast<uint16_t>(0));
}

void GpsTest::_testRTCMParserPreambleConstant()
{
    QCOMPARE_EQ(RTCM3_PREAMBLE, 0xD3);
}

void GpsTest::_testRTCMParserCrcSize()
{
    RTCMParser parser;
    QCOMPARE_EQ(parser.crcSize(), 3);
}

void GpsTest::_testRTCMParserInitialState()
{
    RTCMParser parser;
    parser.reset();
    QCOMPARE_EQ(parser.messageLength(), static_cast<uint16_t>(0));
    VERIFY_NOT_NULL(parser.message());
}

void GpsTest::_testRTCMParserAddPreambleByte()
{
    RTCMParser parser;
    parser.reset();

    bool result = parser.addByte(RTCM3_PREAMBLE);
    QVERIFY(!result);

    result = parser.addByte(0x00);
    QVERIFY(!result);
}

// ============================================================================
// GPSRTKFactGroup Tests
// ============================================================================

void GpsTest::_testGPSRTKFactGroupCreation()
{
    GPSRTKFactGroup* factGroup = new GPSRTKFactGroup(this);
    VERIFY_NOT_NULL(factGroup);
    delete factGroup;
}

void GpsTest::_testGPSRTKFactGroupConnectedFact()
{
    GPSRTKFactGroup factGroup(this);
    Fact* fact = factGroup.connected();
    VERIFY_NOT_NULL(fact);
    QCOMPARE(fact->name(), QStringLiteral("connected"));
    QCOMPARE(fact->type(), FactMetaData::valueTypeBool);
}

void GpsTest::_testGPSRTKFactGroupDurationFact()
{
    GPSRTKFactGroup factGroup(this);
    Fact* fact = factGroup.currentDuration();
    VERIFY_NOT_NULL(fact);
    QCOMPARE(fact->name(), QStringLiteral("currentDuration"));
    QCOMPARE(fact->type(), FactMetaData::valueTypeDouble);
}

void GpsTest::_testGPSRTKFactGroupAccuracyFact()
{
    GPSRTKFactGroup factGroup(this);
    Fact* fact = factGroup.currentAccuracy();
    VERIFY_NOT_NULL(fact);
    QCOMPARE(fact->name(), QStringLiteral("currentAccuracy"));
    QCOMPARE(fact->type(), FactMetaData::valueTypeDouble);
}

void GpsTest::_testGPSRTKFactGroupLatitudeFact()
{
    GPSRTKFactGroup factGroup(this);
    Fact* fact = factGroup.currentLatitude();
    VERIFY_NOT_NULL(fact);
    QCOMPARE(fact->name(), QStringLiteral("currentLatitude"));
    QCOMPARE(fact->type(), FactMetaData::valueTypeDouble);
}

void GpsTest::_testGPSRTKFactGroupLongitudeFact()
{
    GPSRTKFactGroup factGroup(this);
    Fact* fact = factGroup.currentLongitude();
    VERIFY_NOT_NULL(fact);
    QCOMPARE(fact->name(), QStringLiteral("currentLongitude"));
    QCOMPARE(fact->type(), FactMetaData::valueTypeDouble);
}

void GpsTest::_testGPSRTKFactGroupAltitudeFact()
{
    GPSRTKFactGroup factGroup(this);
    Fact* fact = factGroup.currentAltitude();
    VERIFY_NOT_NULL(fact);
    QCOMPARE(fact->name(), QStringLiteral("currentAltitude"));
    QCOMPARE(fact->type(), FactMetaData::valueTypeFloat);
}

void GpsTest::_testGPSRTKFactGroupValidFact()
{
    GPSRTKFactGroup factGroup(this);
    Fact* fact = factGroup.valid();
    VERIFY_NOT_NULL(fact);
    QCOMPARE(fact->name(), QStringLiteral("valid"));
    QCOMPARE(fact->type(), FactMetaData::valueTypeBool);
}

void GpsTest::_testGPSRTKFactGroupActiveFact()
{
    GPSRTKFactGroup factGroup(this);
    Fact* fact = factGroup.active();
    VERIFY_NOT_NULL(fact);
    QCOMPARE(fact->name(), QStringLiteral("active"));
    QCOMPARE(fact->type(), FactMetaData::valueTypeBool);
}

void GpsTest::_testGPSRTKFactGroupNumSatellitesFact()
{
    GPSRTKFactGroup factGroup(this);
    Fact* fact = factGroup.numSatellites();
    VERIFY_NOT_NULL(fact);
    QCOMPARE(fact->name(), QStringLiteral("numSatellites"));
    QCOMPARE(fact->type(), FactMetaData::valueTypeInt32);
}

// ============================================================================
// Conversion Constant Tests
// ============================================================================

void GpsTest::_testDegToRadConversion()
{
    QVERIFY(qFuzzyCompare(M_DEG_TO_RAD, M_PI / 180.0));

    double degrees = 180.0;
    double radians = degrees * M_DEG_TO_RAD;
    QVERIFY(qFuzzyCompare(radians, M_PI));
}

void GpsTest::_testRadToDegConversion()
{
    QVERIFY(qFuzzyCompare(M_RAD_TO_DEG, 180.0 / M_PI));

    double radians = M_PI;
    double degrees = radians * M_RAD_TO_DEG;
    QVERIFY(qFuzzyCompare(degrees, 180.0));
}

void GpsTest::_testDegToRadFloatConversion()
{
    QVERIFY(qAbs(M_DEG_TO_RAD_F - 0.0174532925f) < 0.0000001f);

    float degrees = 90.0f;
    float radians = degrees * M_DEG_TO_RAD_F;
    QVERIFY(qAbs(radians - static_cast<float>(M_PI / 2.0)) < 0.0001f);
}

void GpsTest::_testRadToDegFloatConversion()
{
    QVERIFY(qAbs(M_RAD_TO_DEG_F - 57.2957795f) < 0.0000001f);

    float radians = static_cast<float>(M_PI / 2.0);
    float degrees = radians * M_RAD_TO_DEG_F;
    QVERIFY(qAbs(degrees - 90.0f) < 0.001f);
}
