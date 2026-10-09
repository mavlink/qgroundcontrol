#pragma once

#include <utility>

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtTest/QTest>

#include "GPSReceiver.h"
#include "GPSReceiverFactGroup.h"
#include "Receiver/Support/ScriptedReceiverWorker.h"
#ifndef QGC_NO_SERIAL_LINK
#include "SerialPortManager.h"
#endif

/// Receiver configurations, a receiver whose worker is scripted, and the serial devices it sees, shared by the receiver
/// suites.
namespace GPSTest {
inline GPSPositionReport fixReport(GPSFixQuality fixType)
{
    GPSPositionReport report;
    report.navigation.fixType = fixType;
    return report;
}

/// The descriptor ID of the passive family, which the settings select by role rather than by manufacturer.
inline const int kPassiveManufacturer = gpsReceiverManufacturerForType(GPSType::passive);

inline GPSReceiver::Configuration receiverConfiguration(
    int manufacturer = gpsReceiverManufacturerForType(GPSType::ublox))
{
    // The settings' defaults.
    GPSReceiver::Configuration configuration;
    configuration.udpPort = 14401;
    configuration.serialBaudRate = 115200;
    configuration.base.mode =
        GPSBaseStationConfig::SurveyIn{.accuracyMeters = 2., .duration = std::chrono::seconds{180}};
    if (manufacturer == kPassiveManufacturer) {
        configuration.receiverRole = RTKSettings::Passive;
    } else {
        configuration.receiverRole = RTKSettings::ConfiguredBase;
        configuration.baseReceiverManufacturer = manufacturer;
    }
    return configuration;
}

/// Connects @a configuration over TCP; scripted workers never open the address.
inline void useTcp(GPSReceiver::Configuration& configuration)
{
    configuration.connectionType = RTKSettings::Tcp;
    configuration.tcpHost = QStringLiteral("rtk.test");
    configuration.tcpPort = 2101;
}

/// Connects @a configuration over TCP as the user does, without automatic reconnection.
inline bool connectOverTcp(GPSReceiver& receiver, GPSReceiver::Configuration configuration = receiverConfiguration())
{
    useTcp(configuration);
    configuration.autoConnect = false;
    receiver.setConfiguration(configuration);
    return receiver.connectReceiver();
}

/// Real workers whose transports come from @a transportFactory instead of the connection's address.
inline GPSReceiver::WorkerFactory workerFactory(GPSReceiverWorker::TransportFactory transportFactory)
{
    return
        [transportFactory](GPSReceiverWorker::TransportFactory, GPSType type, const GPSReceiverConfig& config,
                           QObject* parent) { return new GPSReceiverWorker(transportFactory, type, config, parent); };
}

/// The receiver's solution as its status Facts hold it: fix, satellite counts, and integrity states.
struct Solution
{
    GPSFixQuality fixType = GPSFixQuality::Unknown;
    int inView = -1;
    int used = -1;
    int jamming = 0;
    int spoofing = 0;
    int antenna = 0;

    bool operator==(const Solution&) const = default;
};

inline Solution solution(GPSReceiverFactGroup& facts)
{
    return {.fixType = gpsFixQualityFromValue(facts.fixType()->rawValue().toInt()),
            .inView = facts.numSatellites()->rawValue().toInt(),
            .used = facts.numSatellitesUsed()->rawValue().toInt(),
            .jamming = facts.jammingState()->rawValue().toInt(),
            .spoofing = facts.spoofingState()->rawValue().toInt(),
            .antenna = facts.antennaState()->rawValue().toInt()};
}

/// Found by QCOMPARE through argument-dependent lookup.
inline char* toString(const Solution& solution)
{
    return QTest::toString(QStringLiteral("fix %1, satellites %2/%3, jamming %4, spoofing %5, antenna %6")
                               .arg(static_cast<int>(solution.fixType))
                               .arg(solution.used)
                               .arg(solution.inView)
                               .arg(solution.jamming)
                               .arg(solution.spoofing)
                               .arg(solution.antenna));
}

#ifndef QGC_NO_SERIAL_LINK
inline GPSReceiver::Configuration serialConfiguration(int manufacturer, const QString& device, uint32_t baudRate)
{
    auto configuration = receiverConfiguration(manufacturer);
    configuration.connectionType = RTKSettings::Serial;
    configuration.serialDevice = device;
    configuration.serialBaudRate = baudRate;
    return configuration;
}

/// A serial device without USB identity, such as a generic USB serial adapter.
inline SerialPortManager::Port serialPort(const QString& location)
{
    return {.systemLocation = location, .portName = location.section(u'/', -1), .boardName = {}};
}

/// A device whose USB identity is a known RTK receiver, which discovery may connect.
inline SerialPortManager::Port rtkPort(const QString& location = QStringLiteral("/test/rtk"))
{
    SerialPortManager::Port port = serialPort(location);
    port.boardType = QGCSerialPortInfo::BoardTypeRTKGPS;
    port.boardName = QStringLiteral("u-blox");
    return port;
}

/// Serial devices the test plugs in and unplugs, listed by a manager that scans them instead of the system's.
struct TestSerialPorts
{
    explicit TestSerialPorts(QList<SerialPortManager::Port> ports = {})
        : listed(std::move(ports))
    {}

    TestSerialPorts(const TestSerialPorts&) = delete;
    TestSerialPorts& operator=(const TestSerialPorts&) = delete;

    /// Lists @a ports from now on, as plugging and unplugging devices does, and scans them at once.
    void plug(QList<SerialPortManager::Port> ports)
    {
        listed = std::move(ports);
        manager.rescan();
    }

    QList<SerialPortManager::Port> listed;
    SerialPortManager manager{nullptr, [this] { return listed; }};
};
#endif

struct ScriptedGPSReceiver
{
    explicit ScriptedGPSReceiver(RuntimeScheduler* scheduler = nullptr,
                                 const GPSReceiver::Dependencies& dependencies = {})
        : receiver(nullptr, scheduler, dependencies)
    {
        receiver.setWorkerFactory(workers.workerFactory());
    }

    GPSReceiver receiver;
    ScriptedReceiverWorkerFactory workers;
};
}  // namespace GPSTest
