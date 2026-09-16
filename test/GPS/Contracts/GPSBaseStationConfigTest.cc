#include <limits>

#include <QtTest/QTest>

#include "GPSReceiverConfig.h"

Q_DECLARE_METATYPE(GPSReceiverConfig)

class GPSBaseStationConfigTest : public QObject
{
    Q_OBJECT

private slots:

    void _baseValidation_data()
    {
        QTest::addColumn<GPSReceiverConfig>("config");
        QTest::addColumn<bool>("valid");
        const auto survey = [](const char* name, double accuracy, int64_t duration, bool valid) {
            QTest::newRow(name) << GPSReceiverConfig{.base = {.surveyInAccMeters = accuracy,
                                                              .surveyInDurationSecs = duration}}
                                << valid;
        };
        survey("minimum", 0.0001, 1, true);
        survey("maximum", 429496.7295, 4294967295LL, true);
        survey("accuracy-underflow", 0.00001, 1, false);
        survey("accuracy-overflow", 429496.7296, 1, false);
        survey("accuracy-nan", qQNaN(), 1, false);
        survey("duration-zero", 1, 0, false);
        survey("duration-overflow", 1, 4294967296LL, false);
        const auto fixed = [](const char* name, double latitude, double longitude, float altitude, float accuracy,
                              bool valid) {
            QTest::newRow(name) << GPSReceiverConfig{.base = {.useFixedBase = true,
                                                              .fixedBaseLatitude = latitude,
                                                              .fixedBaseLongitude = longitude,
                                                              .fixedBaseAltitudeMeters = altitude,
                                                              .fixedBaseAccuracyMeters = accuracy}}
                                << valid;
        };
        fixed("fixed-unknown-accuracy", 47, 8, 500, 0, true);
        fixed("fixed-wire-limit", 47, 8, 21474836.0f, 429496.71875f, true);
        fixed("invalid-latitude", 91, 8, 500, 1, false);
        fixed("invalid-longitude", 47, -181, 500, 1, false);
        fixed("nan-latitude", qQNaN(), 8, 500, 1, false);
        fixed("nan-longitude", 47, qQNaN(), 500, 1, false);
        fixed("altitude-overflow", 47, 8, 21474838.0f, 1, false);
        fixed("negative-accuracy", 47, 8, 500, -1, false);
        fixed("infinite-accuracy", 47, 8, 500, std::numeric_limits<float>::infinity(), false);
    }

    void _baseValidation()
    {
        QFETCH(GPSReceiverConfig, config);
        QFETCH(bool, valid);
        QCOMPARE(config.validationError().isEmpty(), valid);
    }

    void _configurationShapeValidation_data()
    {
        QTest::addColumn<GPSReceiverConfig>("config");
        QTest::addColumn<bool>("valid");
        const GPSReceiverConfig base{.base = {.surveyInAccMeters = 1, .surveyInDurationSecs = 60}};
        QTest::newRow("valid-base") << base << true;
        QTest::newRow("missing-survey") << GPSReceiverConfig{} << false;
        auto config = base;
        config.role = static_cast<GPSReceiverConfig::Role>(-1);
        QTest::newRow("invalid-role") << config << false;
        config = base;
        config.outputProtocol = GPSReceiverConfig::OutputProtocol::NMEA;
        QTest::newRow("base-nmea") << config << false;
        config.outputProtocol = static_cast<GPSReceiverConfig::OutputProtocol>(-1);
        QTest::newRow("invalid-protocol") << config << false;
        config = base;
        config.headingOffsetDeg = qQNaN();
        QTest::newRow("nan-heading") << config << false;
        config.headingOffsetDeg = std::numeric_limits<float>::infinity();
        QTest::newRow("infinite-heading") << config << false;
        config = base;
        config.constellationMask = 2;
        config.dynamicModel = 4;
        config.outputRateHz = 5;
        config.headingOffsetDeg = 12;
        QTest::newRow("driver-settings-not-shape-constraints") << config << true;
        config.base.surveyInAccMeters = -1;
        QTest::newRow("settings-do-not-bypass-base-validation") << config << false;
        config.role = GPSReceiverConfig::Role::Position;
        QTest::newRow("position-ignores-base") << config << true;
        config.outputProtocol = GPSReceiverConfig::OutputProtocol::NMEA;
        QTest::newRow("position-nmea") << config << true;
        config.headingOffsetDeg = qQNaN();
        QTest::newRow("position-nan-heading") << config << false;
    }

    void _configurationShapeValidation()
    {
        QFETCH(GPSReceiverConfig, config);
        QFETCH(bool, valid);
        QCOMPARE(config.validationError().isEmpty(), valid);
    }

    void _surveyWireRange()
    {
        GPSReceiverConfig config{.base = {.surveyInAccMeters = 429496.7295, .surveyInDurationSecs = 4294967295LL}};
        QVERIFY(config.validationError().isEmpty());
        QCOMPARE(static_cast<uint32_t>(config.base.surveyInAccMeters * 10000.0),
                 (std::numeric_limits<uint32_t>::max)());
        QCOMPARE(static_cast<uint32_t>(config.base.surveyInDurationSecs), (std::numeric_limits<uint32_t>::max)());
        ++config.base.surveyInDurationSecs;
        QVERIFY(!config.validationError().isEmpty());
    }

    void _fixedWireRepresentability()
    {
        GPSReceiverConfig config{.base = {.useFixedBase = true,
                                          .fixedBaseLatitude = 47,
                                          .fixedBaseLongitude = 8,
                                          .fixedBaseAltitudeMeters = 21474836.0f,
                                          .fixedBaseAccuracyMeters = 429496.71875f}};
        QVERIFY(config.validationError().isEmpty());
        QCOMPARE(static_cast<int32_t>(static_cast<double>(config.base.fixedBaseAltitudeMeters) * 100.0), 2147483600);
        const float accuracyMillimeters = config.base.fixedBaseAccuracyMeters * 1000.0f;
        QCOMPARE(static_cast<uint32_t>(accuracyMillimeters * 10.0f), 4294967040u);
        config.base.fixedBaseAccuracyMeters = 429496.75f;
        QVERIFY(!config.validationError().isEmpty());
        config.base.fixedBaseAccuracyMeters = 0;
        QVERIFY(config.validationError().isEmpty());
    }

};

QTEST_APPLESS_MAIN(GPSBaseStationConfigTest)
#include "GPSBaseStationConfigTest.moc"
