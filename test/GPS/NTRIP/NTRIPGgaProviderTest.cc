#include "NTRIPGgaProviderTest.h"

#include <optional>

#include <QtPositioning/QGeoCoordinate>
#include <QtTest/QTest>

#include "MockNTRIPTransport.h"
#include "NMEASentence.h"
#include "NMEAUtils.h"
#include "NTRIPGgaProvider.h"

namespace {

using Source = NTRIPGgaProvider::PositionSource;

}  // namespace

void NTRIPGgaProviderTest::testSourceClearedOnStopAndFreshStart()
{
    NTRIPGgaProvider provider;
    MockNTRIPTransport transport;

    provider.setPositionProvider(NTRIPGgaProvider::PositionSource::VehicleGPS, []() {
        return PositionResult{QGeoCoordinate(47.3977, 8.5456, 450.0), QStringLiteral("Vehicle GPS"),
                              GPSAltitudeDatum::MeanSeaLevel};
    });

    provider.start(&transport);
    QCOMPARE(provider.currentSource(), QStringLiteral("Vehicle GPS"));
    QCOMPARE(transport.sentNmea.size(), 1);
    QVERIFY(transport.sentNmea.first().startsWith("$GPGGA,"));
    const auto fields = transport.sentNmea.first().split(',');
    QCOMPARE(fields.at(NMEA::Field::GGA_QUALITY).toUInt(), NMEA::GgaQuality::ESTIMATED);
    QVERIFY(fields.at(NMEA::Field::GGA_HDOP).isEmpty());
    QVERIFY(fields.at(NMEA::Field::GGA_SATELLITES_USED).isEmpty());

    provider.stop();
    QVERIFY(provider.currentSource().isEmpty());

    provider.setPositionProvider(NTRIPGgaProvider::PositionSource::VehicleGPS, []() { return PositionResult{}; });
    provider.start(&transport);
    QVERIFY(provider.currentSource().isEmpty());
}

void NTRIPGgaProviderTest::testRTKReceiverProvider()
{
    bool receiverValid = false;
    NTRIPGgaProvider provider;
    MockNTRIPTransport transport;
    provider.configure({Source::RTKReceiver});
    provider.setPositionProvider(Source::RTKReceiver, [&]() {
        return receiverValid ? PositionResult{QGeoCoordinate(47.3977, 8.5456, 450.0),
                                              QStringLiteral("RTK Receiver"),
                                              GPSAltitudeDatum::MeanSeaLevel,
                                              GPSObservation::FixQuality::RTKFixed,
                                              21,
                                              0.7}
                             : PositionResult{};
    });

    provider.start(&transport);
    QVERIFY(provider.currentSource().isEmpty());
    QVERIFY(transport.sentNmea.isEmpty());
    provider.stop();

    receiverValid = true;
    provider.start(&transport);
    QCOMPARE(provider.currentSource(), QStringLiteral("RTK Receiver"));
    QCOMPARE(transport.sentNmea.size(), 1);
    const auto& wire = transport.sentNmea.first();
    const auto decoded = NMEA::sentence(std::string_view(wire.constData(), wire.size()));
    QVERIFY(decoded);
    const auto fix = NMEA::gga(*decoded);
    QVERIFY(fix);
    QVERIFY(qAbs(fix->latitude - 47.3977) < 1e-6);
    QVERIFY(qAbs(fix->longitude - 8.5456) < 1e-6);
    QCOMPARE(fix->quality, NMEA::GgaQuality::RTK_FIXED);
    QCOMPARE(fix->satellitesUsed, std::optional<unsigned>(21));
    provider.stop();

    receiverValid = false;
    transport.sentNmea.clear();
    provider.start(&transport);
    QVERIFY(provider.currentSource().isEmpty());
    QVERIFY(transport.sentNmea.isEmpty());
}

void NTRIPGgaProviderTest::_invalidProviderAltitude_data()
{
    QTest::addColumn<double>("altitude");
    QTest::newRow("unknown") << qQNaN();
    QTest::newRow("positive-infinity") << qInf();
    QTest::newRow("negative-infinity") << -qInf();
}

void NTRIPGgaProviderTest::_invalidProviderAltitude()
{
    QFETCH(double, altitude);
    NTRIPGgaProvider provider;
    MockNTRIPTransport transport;
    provider.setPositionProvider(Source::VehicleGPS, [altitude]() {
        return PositionResult{QGeoCoordinate(47, 8, altitude), QStringLiteral("Vehicle GPS"),
                              GPSAltitudeDatum::MeanSeaLevel};
    });
    provider.setPositionProvider(Source::GCSPosition, []() {
        return PositionResult{QGeoCoordinate(48, 9, 0), QStringLiteral("GCS"), GPSAltitudeDatum::MeanSeaLevel};
    });

    provider.configure({Source::VehicleGPS});
    provider.start(&transport);
    QVERIFY(transport.sentNmea.isEmpty());
    QVERIFY(provider.currentSource().isEmpty());
    provider.stop();

    provider.configure({Source::Auto});
    provider.start(&transport);
    QCOMPARE(transport.sentNmea.size(), 1);
    QCOMPARE(provider.currentSource(), QStringLiteral("GCS"));
    const auto fields = transport.sentNmea.first().split(',');
    QCOMPARE(fields.size(), 15);
    QCOMPARE(fields.at(NMEA::Field::GGA_ALTITUDE), QByteArray("0.0"));
    QCOMPARE(fields.at(NMEA::Field::GGA_ALTITUDE_UNITS), QByteArray("M"));
    QVERIFY(fields.at(NMEA::Field::GGA_GEOID_SEPARATION).isEmpty());
    QVERIFY(NMEAUtils::verifyChecksum(transport.sentNmea.first()));
}

void NTRIPGgaProviderTest::_gcsObservation_data()
{
    QTest::addColumn<QGeoCoordinate>("coordinate");
    QTest::addColumn<GPSAltitudeDatum>("datum");
    QTest::addColumn<double>("horizontalAccuracy");
    QTest::addColumn<bool>("fixValid");
    QTest::addColumn<bool>("accepted");
    const QGeoCoordinate position(47.3977, 8.5456, 450);
    QTest::newRow("missing-vertical-accuracy") << position << GPSAltitudeDatum::MeanSeaLevel << 1.0 << true << true;
    QTest::newRow("missing-all-accuracy") << position << GPSAltitudeDatum::MeanSeaLevel << qQNaN() << true << true;
    QTest::newRow("unknown-datum") << position << GPSAltitudeDatum::Unknown << 1.0 << true << false;
    QTest::newRow("ellipsoid-only") << position << GPSAltitudeDatum::Ellipsoid << 1.0 << true << false;
    QTest::newRow("unknown-altitude") << QGeoCoordinate(47.3977, 8.5456) << GPSAltitudeDatum::MeanSeaLevel << 1.0
                                      << true << false;
    QTest::newRow("zero-island") << QGeoCoordinate(0, 0, 450) << GPSAltitudeDatum::MeanSeaLevel << 1.0 << true << true;
    QTest::newRow("invalid-coordinate") << QGeoCoordinate() << GPSAltitudeDatum::MeanSeaLevel << 1.0 << true << false;
    QTest::newRow("no-fix") << position << GPSAltitudeDatum::MeanSeaLevel << 1.0 << false << false;
}

void NTRIPGgaProviderTest::_gcsObservation()
{
    QFETCH(QGeoCoordinate, coordinate);
    QFETCH(GPSAltitudeDatum, datum);
    QFETCH(double, horizontalAccuracy);
    QFETCH(bool, fixValid);
    QFETCH(bool, accepted);
    NTRIPGgaProvider provider;
    MockNTRIPTransport transport;
    provider.configure({Source::GCSPosition});
    provider.setPositionProvider(Source::GCSPosition, [&]() {
        return PositionResult{
            coordinate,   QStringLiteral("GCS Position"),
            datum,        fixValid ? GPSObservation::FixQuality::Fix3D : GPSObservation::FixQuality::NoFix,
            std::nullopt, qIsFinite(horizontalAccuracy) ? std::optional<double>(horizontalAccuracy) : std::nullopt};
    });
    provider.start(&transport);
    QCOMPARE(transport.sentNmea.size(), accepted ? 1 : 0);
    QCOMPARE(provider.currentSource(), accepted ? QStringLiteral("GCS Position") : QString());
    if (accepted) {
        const auto& wire = transport.sentNmea.first();
        const auto decoded = NMEA::sentence(std::string_view(wire.constData(), wire.size()));
        QVERIFY(decoded);
        const auto fix = NMEA::gga(*decoded);
        QVERIFY(fix);
        QVERIFY(qAbs(fix->latitude - coordinate.latitude()) < 1e-6);
        QVERIFY(qAbs(fix->longitude - coordinate.longitude()) < 1e-6);
        const auto fields = transport.sentNmea.first().split(',');
        QCOMPARE(fields.size(), 15);
        QCOMPARE(fields.at(NMEA::Field::GGA_ALTITUDE), QByteArray("450.0"));
        QCOMPARE(fields.at(NMEA::Field::GGA_ALTITUDE_UNITS), QByteArray("M"));
        QVERIFY(fields.at(NMEA::Field::GGA_GEOID_SEPARATION).isEmpty());
        QVERIFY(NMEAUtils::verifyChecksum(transport.sentNmea.first()));
    }
}

void NTRIPGgaProviderTest::_providerMetadata_data()
{
    QTest::addColumn<GPSObservation::FixQuality>("quality");
    QTest::addColumn<unsigned>("expectedQuality");
    using Quality = GPSObservation::FixQuality;
    QTest::newRow("gps") << Quality::Fix3D << NMEA::GgaQuality::GPS;
    QTest::newRow("differential") << Quality::Differential << NMEA::GgaQuality::DIFFERENTIAL;
    QTest::newRow("rtk-float") << Quality::RTKFloat << NMEA::GgaQuality::RTK_FLOAT;
    QTest::newRow("rtk-fixed") << Quality::RTKFixed << NMEA::GgaQuality::RTK_FIXED;
    QTest::newRow("estimated") << Quality::Extrapolated << NMEA::GgaQuality::ESTIMATED;
    QTest::newRow("unknown") << Quality::Unknown << NMEA::GgaQuality::ESTIMATED;
}

void NTRIPGgaProviderTest::_providerMetadata()
{
    QFETCH(GPSObservation::FixQuality, quality);
    QFETCH(unsigned, expectedQuality);
    NTRIPGgaProvider provider;
    MockNTRIPTransport transport;
    provider.setPositionProvider(Source::VehicleGPS, [quality]() {
        return PositionResult{
            QGeoCoordinate(47, 8, 450), QStringLiteral("GPS"), GPSAltitudeDatum::MeanSeaLevel, quality, 18, 0.7};
    });
    provider.start(&transport);
    QCOMPARE(transport.sentNmea.size(), 1);
    const auto fields = transport.sentNmea.first().split(',');
    QCOMPARE(fields.at(NMEA::Field::GGA_QUALITY).toUInt(), expectedQuality);
    QCOMPARE(fields.at(NMEA::Field::GGA_SATELLITES_USED).toUInt(), 18u);
    QCOMPARE(fields.at(NMEA::Field::GGA_HDOP).toDouble(), 0.7);
}

UT_REGISTER_TEST(NTRIPGgaProviderTest, TestLabel::Unit)
