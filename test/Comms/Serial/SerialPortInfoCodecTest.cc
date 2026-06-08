// SPDX-License-Identifier: Apache-2.0 OR GPL-3.0-only

#include "SerialPortInfoCodecTest.h"

#include <QtCore/QByteArray>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include "SerialPortInfoCodec.h"

namespace {

QByteArray buildPorts(const QJsonArray& ports)
{
    QJsonObject root;
    root["ports"] = ports;
    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

}  // namespace

void SerialPortInfoCodecTest::_emptyBuffer_returnsEmpty()
{
    QVERIFY(SerialPortInfoCodec::unpack(QByteArray()).isEmpty());
}

void SerialPortInfoCodecTest::_garbledBuffer_returnsEmpty()
{
    QVERIFY(SerialPortInfoCodec::unpack(QByteArrayLiteral("not json")).isEmpty());
}

void SerialPortInfoCodecTest::_singlePort_decodesAllFields()
{
    QJsonObject port;
    port["deviceName"] = "/dev/ttyACM0";
    port["productName"] = "Pixhawk";
    port["manufacturerName"] = "ArduPilot";
    port["serialNumber"] = "SN123";
    port["productId"] = 0x0011;
    port["vendorId"] = 0x26ac;
    port["baudRates"] = QJsonArray{57600, 115200, 921600};

    const QList<QGCSerialPortInfo::Data> ports = SerialPortInfoCodec::unpack(buildPorts({port}));
    QCOMPARE(ports.size(), 1);
    const QGCSerialPortInfo::Data& d = ports.first();
    QCOMPARE(d.systemLocation, QStringLiteral("/dev/ttyACM0"));
    QCOMPARE(d.portName, QStringLiteral("ttyACM0"));  // /dev/ stripped
    QCOMPARE(d.description, QStringLiteral("Pixhawk"));
    QCOMPARE(d.manufacturer, QStringLiteral("ArduPilot"));
    QCOMPARE(d.serialNumber, QStringLiteral("SN123"));
    QCOMPARE(d.productIdentifier, static_cast<quint16>(0x0011));
    QCOMPARE(d.vendorIdentifier, static_cast<quint16>(0x26ac));
    QVERIFY(d.hasProductIdentifier);
    QVERIFY(d.hasVendorIdentifier);
    QCOMPARE(d.supportedBaudRates, (QList<qint32>{57600, 115200, 921600}));
}

void SerialPortInfoCodecTest::_absentStringKey_isNullDistinctFromEmpty()
{
    QJsonObject port;
    port["deviceName"] = "/dev/ttyUSB0";
    port["manufacturerName"] = "";  // present-but-empty
    // productName, serialNumber absent -> null QString
    port["productId"] = 1;
    port["vendorId"] = 2;
    port["baudRates"] = QJsonArray{};

    const QList<QGCSerialPortInfo::Data> ports = SerialPortInfoCodec::unpack(buildPorts({port}));
    QCOMPARE(ports.size(), 1);
    const QGCSerialPortInfo::Data& d = ports.first();
    QVERIFY(d.description.isNull());        // absent key
    QVERIFY(d.manufacturer.isEmpty());      // present empty
    QVERIFY(!d.manufacturer.isNull());
    QVERIFY(d.serialNumber.isNull());
    QVERIFY(d.supportedBaudRates.isEmpty());
}

void SerialPortInfoCodecTest::_emptyDeviceName_skipsPortButKeepsParsing()
{
    QJsonObject ghost;
    ghost["productName"] = "ghost";  // no deviceName -> dropped
    ghost["baudRates"] = QJsonArray{9600, 19200};

    QJsonObject valid;
    valid["deviceName"] = "/dev/ttyUSB0";
    valid["productName"] = "FTDI";
    valid["manufacturerName"] = "FT";
    valid["serialNumber"] = "S2";
    valid["baudRates"] = QJsonArray{57600};

    const QList<QGCSerialPortInfo::Data> ports = SerialPortInfoCodec::unpack(buildPorts({ghost, valid}));
    QCOMPARE(ports.size(), 1);
    QCOMPARE(ports.first().systemLocation, QStringLiteral("/dev/ttyUSB0"));
    QCOMPARE(ports.first().supportedBaudRates, (QList<qint32>{57600}));
}

// Decodes the same on-disk fixture the Java UsbPortInfoPackingTest packs against, so a JSON key rename on
// either side drifts from the shared golden literal and fails one suite.
void SerialPortInfoCodecTest::_goldenFixture_matchesSharedContract()
{
    QFile golden(QStringLiteral(SERIAL_TESTDATA_DIR "/PortInfoGolden.json"));
    QVERIFY2(golden.open(QIODevice::ReadOnly), "golden fixture missing — check SERIAL_TESTDATA_DIR");

    const QList<QGCSerialPortInfo::Data> ports = SerialPortInfoCodec::unpack(golden.readAll());
    QCOMPARE(ports.size(), 1);
    const QGCSerialPortInfo::Data& d = ports.first();
    QCOMPARE(d.systemLocation, QStringLiteral("/dev/ttyACM0"));
    QCOMPARE(d.description, QStringLiteral("Pixhawk"));
    QCOMPARE(d.manufacturer, QStringLiteral("ArduPilot"));
    QCOMPARE(d.serialNumber, QStringLiteral("SN123"));
    QCOMPARE(d.productIdentifier, static_cast<quint16>(0x0011));
    QCOMPARE(d.vendorIdentifier, static_cast<quint16>(0x26ac));
    QCOMPARE(d.supportedBaudRates, (QList<qint32>{57600, 115200, 921600}));
}

UT_REGISTER_TEST(SerialPortInfoCodecTest, TestLabel::Unit, TestLabel::Comms)
