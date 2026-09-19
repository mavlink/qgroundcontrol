#include <limits>

#include <QtTest/QTest>

#include "GPSReceiverConfigValidation.h"
#include "PortableTest.h"

Q_DECLARE_METATYPE(GPSBaseStationConfig)

class GPSBaseStationConfigTest : public PortableTest
{
    Q_OBJECT

private slots:

    void _baseValidation_data()
    {
        QTest::addColumn<GPSBaseStationConfig>("config");
        QTest::addColumn<bool>("valid");
        const auto survey = [](const char* name, double accuracy, int64_t duration, bool valid) {
            QTest::newRow(name) << GPSBaseStationConfig{.surveyInAccMeters = accuracy, .surveyInDurationSecs = duration}
                                << valid;
        };
        QTest::newRow("missing-survey") << GPSBaseStationConfig{} << false;
        survey("minimum", 0.0001, 1, true);
        survey("maximum", 429496.7295, 4294967295LL, true);
        survey("negative-accuracy", -1, 1, false);
        survey("accuracy-underflow", 0.00001, 1, false);
        survey("accuracy-overflow", 429496.7296, 1, false);
        survey("accuracy-nan", qQNaN(), 1, false);
        survey("accuracy-infinite", qInf(), 1, false);
        survey("duration-negative", 1, -1, false);
        survey("duration-zero", 1, 0, false);
        survey("duration-overflow", 1, 4294967296LL, false);
        survey("duration-int64-overflow", 1, (std::numeric_limits<int64_t>::max)(), false);
        const auto fixed = [](const char* name, double latitude, double longitude, float altitude, float accuracy,
                              bool valid) {
            QTest::newRow(name) << GPSBaseStationConfig{.useFixedBase = true,
                                                        .fixedBaseLatitude = latitude,
                                                        .fixedBaseLongitude = longitude,
                                                        .fixedBaseAltitudeMeters = altitude,
                                                        .fixedBaseAccuracyMeters = accuracy}
                                << valid;
        };
        QTest::newRow("missing-fixed-position") << GPSBaseStationConfig{.useFixedBase = true} << false;
        QTest::newRow("missing-fixed-latitude")
            << GPSBaseStationConfig{.useFixedBase = true, .fixedBaseLongitude = 8, .fixedBaseAltitudeMeters = 500}
            << false;
        QTest::newRow("missing-fixed-longitude")
            << GPSBaseStationConfig{.useFixedBase = true, .fixedBaseLatitude = 47, .fixedBaseAltitudeMeters = 500}
            << false;
        QTest::newRow("missing-fixed-altitude")
            << GPSBaseStationConfig{.useFixedBase = true, .fixedBaseLatitude = 47, .fixedBaseLongitude = 8} << false;
        fixed("fixed-explicit-zero-position", 0, 0, 0, 0, true);
        fixed("fixed-unknown-accuracy", 47, 8, 500, 0, true);
        fixed("fixed-wire-limit", 47, 8, 21474836.0f, 429496.71875f, true);
        fixed("fixed-negative-wire-limit", -90, -180, -21474836.0f, 0, true);
        fixed("fixed-positive-coordinate-limits", 90, 180, 0, 0, true);
        fixed("invalid-latitude", 91, 8, 500, 1, false);
        fixed("invalid-longitude", 47, -181, 500, 1, false);
        fixed("nan-latitude", qQNaN(), 8, 500, 1, false);
        fixed("nan-longitude", 47, qQNaN(), 500, 1, false);
        fixed("altitude-overflow", 47, 8, 21474838.0f, 1, false);
        fixed("altitude-underflow", 47, 8, -21474838.0f, 1, false);
        fixed("nan-altitude", 47, 8, std::numeric_limits<float>::quiet_NaN(), 1, false);
        fixed("negative-fixed-accuracy", 47, 8, 500, -1, false);
        fixed("nan-fixed-accuracy", 47, 8, 500, std::numeric_limits<float>::quiet_NaN(), false);
        fixed("infinite-accuracy", 47, 8, 500, std::numeric_limits<float>::infinity(), false);
        fixed("fixed-accuracy-overflow", 47, 8, 500, 429496.75f, false);
    }

    void _baseValidation()
    {
        QFETCH(GPSBaseStationConfig, config);
        QFETCH(bool, valid);
        const QString error = valid                 ? QString()
                              : config.useFixedBase ? QStringLiteral("Enter a valid fixed base position and accuracy")
                                                    : QStringLiteral("Enter a valid survey-in accuracy and duration");
        QCOMPARE(gpsBaseStationConfigError(config), error);
    }

    void _surveyWireRange()
    {
        GPSBaseStationConfig config{.surveyInAccMeters = 429496.7295, .surveyInDurationSecs = 4294967295LL};
        QVERIFY(gpsBaseStationConfigError(config).isEmpty());
        QCOMPARE(static_cast<uint32_t>(config.surveyInAccMeters * 10000.0), (std::numeric_limits<uint32_t>::max)());
        QCOMPARE(static_cast<uint32_t>(config.surveyInDurationSecs), (std::numeric_limits<uint32_t>::max)());
        ++config.surveyInDurationSecs;
        QVERIFY(!gpsBaseStationConfigError(config).isEmpty());
    }

    void _fixedWireRepresentability()
    {
        GPSBaseStationConfig config{.useFixedBase = true,
                                    .fixedBaseLatitude = 47,
                                    .fixedBaseLongitude = 8,
                                    .fixedBaseAltitudeMeters = 21474836.0f,
                                    .fixedBaseAccuracyMeters = 429496.71875f};
        QVERIFY(gpsBaseStationConfigError(config).isEmpty());
        QCOMPARE(static_cast<int32_t>(static_cast<double>(config.fixedBaseAltitudeMeters) * 100.0), 2147483600);
        const float accuracyMillimeters = config.fixedBaseAccuracyMeters * 1000.0f;
        QCOMPARE(static_cast<uint32_t>(accuracyMillimeters * 10.0f), 4294967040u);
        config.fixedBaseAccuracyMeters = 429496.75f;
        QVERIFY(!gpsBaseStationConfigError(config).isEmpty());
        config.fixedBaseAccuracyMeters = 0;
        QVERIFY(gpsBaseStationConfigError(config).isEmpty());
    }
};

QGC_REGISTER_PORTABLE_TEST(GPSBaseStationConfigTest, TestLabel::Unit)
#include "GPSBaseStationConfigTest.moc"
