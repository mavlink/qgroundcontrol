#include <limits>

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtTest/QTest>

#include "GPSReceiverConfig.h"
#include "UnitTest.h"

class GPSBaseStationConfigTest : public UnitTest
{
    Q_OBJECT

private slots:

    void _currentBaseMetadata()
    {
        QFile file(QStringLiteral(":/json/Vehicle/GPSRTKFact.json"));
        QVERIFY(file.open(QIODevice::ReadOnly));
        const auto document = QJsonDocument::fromJson(file.readAll());
        QVERIFY(document.isObject());
        QJsonObject accuracy;
        QJsonObject altitude;
        QJsonObject inView;
        QJsonObject used;
        for (const auto& item : document.object().value("QGC.MetaData.Facts").toArray()) {
            const auto fact = item.toObject();
            if (fact.value("name") == "currentAccuracy") {
                accuracy = fact;
            } else if (fact.value("name") == "currentAltitude") {
                altitude = fact;
            } else if (fact.value("name") == "numSatellites") {
                inView = fact;
            } else if (fact.value("name") == "numSatellitesUsed") {
                used = fact;
            }
        }
        QVERIFY(!accuracy.isEmpty());
        QVERIFY(accuracy.value("default").isNull());
        QVERIFY(accuracy.value("longDesc").toString().contains("Unavailable"));
        QVERIFY(!altitude.isEmpty());
        QVERIFY(altitude.value("default").isNull());
        QVERIFY(altitude.value("longDesc").toString().contains("WGS84 ellipsoid"));
        QVERIFY(!inView.isEmpty());
        QCOMPARE(inView.value("default").toInt(), -1);
        QVERIFY(!used.isEmpty());
        QCOMPARE(used.value("default").toInt(), -1);
    }

    void _baseDiagnostic_data()
    {
        QTest::addColumn<GPSBaseStationConfig>("config");
        QTest::addColumn<QString>("expected");
        QTest::newRow("invalid-survey") << GPSBaseStationConfig{}
                                        << QStringLiteral("Enter a valid survey-in accuracy and duration");
        QTest::newRow("valid-survey") << GPSBaseStationConfig{.mode = GPSBaseStationConfig::SurveyIn{1, 60}}
                                      << QString();
        QTest::newRow("invalid-fixed") << GPSBaseStationConfig{.mode = GPSBaseStationConfig::Fixed{}}
                                       << QStringLiteral("Enter a valid fixed base position and accuracy");
        QTest::newRow("valid-fixed")
            << GPSBaseStationConfig{.mode = GPSBaseStationConfig::Fixed{.position = {.latitudeDegrees = 0,
                                                                                     .longitudeDegrees = 0,
                                                                                     .altitudeMeters = 0}}}
            << QString();
        QTest::newRow("fixed-unavailable-accuracy")
            << GPSBaseStationConfig{.mode = GPSBaseStationConfig::Fixed{.position = {.latitudeDegrees = 47,
                                                                                     .longitudeDegrees = 8,
                                                                                     .altitudeMeters = 500},
                                                                        .accuracyMeters =
                                                                            std::numeric_limits<float>::quiet_NaN()}}
            << QStringLiteral("Enter a valid fixed base position and accuracy");
    }

    void _baseDiagnostic()
    {
        QFETCH(GPSBaseStationConfig, config);
        QFETCH(QString, expected);
        QCOMPARE(gpsReceiverConfigErrorText(gpsValidateBaseStationConfig(config)), expected);
    }

    void _receiverDiagnostic_data()
    {
        QTest::addColumn<GPSType>("type");
        QTest::addColumn<GPSReceiverConfig>("config");
        QTest::addColumn<QString>("expected");
        using Role = GPSReceiverConfig::Role;

        QTest::newRow("valid") << GPSType::ublox
                               << GPSReceiverConfig{.base = {.mode = GPSBaseStationConfig::SurveyIn{1, 60}}}
                               << QString();
        QTest::newRow("unknown-receiver")
            << static_cast<GPSType>(-1) << GPSReceiverConfig{} << QStringLiteral("Unsupported GPS receiver type");
        QTest::newRow("invalid-role") << GPSType::ublox << GPSReceiverConfig{.role = static_cast<Role>(-1)}
                                      << QStringLiteral("Unsupported GPS receiver role");
        QTest::newRow("unsupported-role") << GPSType::ublox << GPSReceiverConfig{.role = Role::Passive}
                                          << QStringLiteral("This receiver does not support the requested role");
        QTest::newRow("invalid-survey") << GPSType::ublox << GPSReceiverConfig{}
                                        << QStringLiteral("Enter a valid survey-in accuracy and duration");
        QTest::newRow("invalid-fixed") << GPSType::ublox
                                       << GPSReceiverConfig{.base = {.mode = GPSBaseStationConfig::Fixed{}}}
                                       << QStringLiteral("Enter a valid fixed base position and accuracy");
        QTest::newRow("unsupported-persistent-configuration")
            << GPSType::ublox
            << GPSReceiverConfig{.base = {.mode = GPSBaseStationConfig::SurveyIn{1, 60}},
                                 .allowPersistentChanges = true}
            << QStringLiteral("This driver does not support persistent receiver configuration");
    }

    void _receiverDiagnostic()
    {
        QFETCH(GPSType, type);
        QFETCH(GPSReceiverConfig, config);
        QFETCH(QString, expected);
        QCOMPARE(gpsReceiverConfigError(type, config), expected);
    }
};

UT_REGISTER_TEST(GPSBaseStationConfigTest, TestLabel::Unit)
#include "GPSBaseStationConfigTest.moc"
