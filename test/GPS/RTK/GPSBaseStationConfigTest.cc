#include <limits>

#include <QtTest/QTest>

#include "GPSReceiverConfigValidation.h"
#include "PortableTest.h"

class GPSBaseStationConfigTest : public PortableTest
{
    Q_OBJECT

private slots:

    void _baseDiagnostic_data()
    {
        QTest::addColumn<GPSBaseStationConfig>("config");
        QTest::addColumn<QString>("expected");
        QTest::newRow("invalid-survey") << GPSBaseStationConfig{}
                                        << QStringLiteral("Enter a valid survey-in accuracy and duration");
        QTest::newRow("valid-survey") << GPSBaseStationConfig{.surveyInAccMeters = 1, .surveyInDurationSecs = 60}
                                      << QString();
        QTest::newRow("invalid-fixed") << GPSBaseStationConfig{.useFixedBase = true}
                                       << QStringLiteral("Enter a valid fixed base position and accuracy");
        QTest::newRow("valid-fixed") << GPSBaseStationConfig{.useFixedBase = true,
                                                             .fixedBaseLatitude = 0,
                                                             .fixedBaseLongitude = 0,
                                                             .fixedBaseAltitudeMeters = 0}
                                     << QString();
    }

    void _baseDiagnostic()
    {
        QFETCH(GPSBaseStationConfig, config);
        QFETCH(QString, expected);
        QCOMPARE(gpsBaseStationConfigError(config), expected);
    }

    void _receiverDiagnostic_data()
    {
        QTest::addColumn<GPSType>("type");
        QTest::addColumn<GPSReceiverConfig>("config");
        QTest::addColumn<QString>("expected");
        using Role = GPSReceiverConfig::Role;

        QTest::newRow("valid") << GPSType::ublox << GPSReceiverConfig{.role = Role::Position} << QString();
        QTest::newRow("unknown-receiver")
            << static_cast<GPSType>(-1) << GPSReceiverConfig{} << QStringLiteral("Unsupported GPS receiver type");
        QTest::newRow("invalid-role") << GPSType::ublox << GPSReceiverConfig{.role = static_cast<Role>(-1)}
                                      << QStringLiteral("Unsupported GPS receiver role");
        QTest::newRow("unsupported-role") << GPSType::septentrio << GPSReceiverConfig{.role = Role::Position}
                                          << QStringLiteral("This receiver does not support the requested role");
        QTest::newRow("invalid-survey") << GPSType::ublox << GPSReceiverConfig{}
                                        << QStringLiteral("Enter a valid survey-in accuracy and duration");
        QTest::newRow("invalid-fixed") << GPSType::ublox << GPSReceiverConfig{.base = {.useFixedBase = true}}
                                       << QStringLiteral("Enter a valid fixed base position and accuracy");
        QTest::newRow("unsupported-constellations")
            << GPSType::septentrio
            << GPSReceiverConfig{.base = {.surveyInAccMeters = 1, .surveyInDurationSecs = 60}, .constellationMask = 1}
            << QStringLiteral("This receiver cannot configure constellations");
        QTest::newRow("invalid-constellations")
            << GPSType::ublox << GPSReceiverConfig{.role = Role::Position, .constellationMask = 32}
            << QStringLiteral("Unsupported constellation selection");
        QTest::newRow("unsupported-dynamic-model")
            << GPSType::septentrio
            << GPSReceiverConfig{.base = {.surveyInAccMeters = 1, .surveyInDurationSecs = 60}, .dynamicModel = 0}
            << QStringLiteral("This receiver role cannot configure a dynamic model");
        QTest::newRow("invalid-dynamic-model")
            << GPSType::ublox << GPSReceiverConfig{.role = Role::Position, .dynamicModel = 1}
            << QStringLiteral("Unsupported receiver dynamic model");
        QTest::newRow("unsupported-heading")
            << GPSType::ublox << GPSReceiverConfig{.role = Role::Position, .headingOffsetRadians = 0.0f}
            << QStringLiteral("This receiver role cannot configure a heading offset");
        QTest::newRow("unsupported-role-precedes-invalid-heading")
            << GPSType::septentrio
            << GPSReceiverConfig{.role = Role::Position,
                                 .headingOffsetRadians = std::numeric_limits<float>::quiet_NaN()}
            << QStringLiteral("This receiver does not support the requested role");
    }

    void _receiverDiagnostic()
    {
        QFETCH(GPSType, type);
        QFETCH(GPSReceiverConfig, config);
        QFETCH(QString, expected);
        QCOMPARE(gpsReceiverConfigError(type, config), expected);
    }
};

QGC_REGISTER_PORTABLE_TEST(GPSBaseStationConfigTest, TestLabel::Unit)
#include "GPSBaseStationConfigTest.moc"
