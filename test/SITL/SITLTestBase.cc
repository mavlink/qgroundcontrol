/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "SITLTestBase.h"

#include "LinkManager.h"
#include "MultiVehicleManager.h"
#include "UDPLink.h"
#include "Vehicle.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QProcess>
#include <QtCore/QStandardPaths>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

Q_LOGGING_CATEGORY(SITLTestLog, "qgc.test.sitl")

SITLTestBase::SITLTestBase(QObject *parent)
    : UnitTest(parent)
{
}

void SITLTestBase::init()
{
    UnitTest::init();
    MultiVehicleManager::instance()->init();

    // Create the UDP listener BEFORE starting the container.
    // On macOS, Docker Desktop and QGC both need port 14550 —
    // binding QGC first with SO_REUSEPORT (via Qt) allows both to coexist.
    QVERIFY2(connectToSITL(), "Failed to create UDP link to SITL");
    QVERIFY2(startContainer(), "Failed to start PX4 SITL container");
    QVERIFY2(waitForVehicle(readinessTimeoutMs()), "Timeout waiting for vehicle heartbeat from SITL");
    QVERIFY2(waitForInitialConnect(initialConnectTimeoutMs()), "Timeout waiting for initial connect sequence");
}

void SITLTestBase::cleanup()
{
    captureContainerLogs();
    stopContainer();
    disconnectFromSITL();

    UnitTest::cleanup();
}

// ----------------------------------------------------------------------------
// Container lifecycle
// ----------------------------------------------------------------------------

QString SITLTestBase::containerImage() const
{
    // Allow override via environment variable for CI
    const QString envImage = qEnvironmentVariable("PX4_SITL_IMAGE");
    if (!envImage.isEmpty()) {
        const QString envDigest = qEnvironmentVariable("PX4_SITL_DIGEST");
        if (!envDigest.isEmpty()) {
            return envImage + QStringLiteral("@") + envDigest;
        }
        return envImage;
    }

    // Fall back to digest file
    const QString digest = _readDigestFile();
    if (!digest.isEmpty()) {
        return QStringLiteral("px4io/px4-sitl-sih@") + digest;
    }

    return QStringLiteral("px4io/px4-sitl-sih:latest");
}

bool SITLTestBase::startContainer()
{
    QProcess docker;
    docker.setProgram(QStringLiteral("docker"));
    // PX4 SITL sends MAVLink to the Docker host gateway on remote port 14550.
    // Docker Desktop maps this via -p so traffic reaches the Mac host.
    // QGC binds to 14550 to receive these packets.
    docker.setArguments({
        QStringLiteral("run"),
        QStringLiteral("--rm"),
        QStringLiteral("-d"),
        QStringLiteral("--name"), QStringLiteral("qgc-sitl-%1-%2")
            .arg(QCoreApplication::applicationPid())
            .arg(QString::fromUtf8(QTest::currentTestFunction())),
        QStringLiteral("-p"), QStringLiteral("%1:14550/udp").arg(mavlinkPort()),
        QStringLiteral("-e"), QStringLiteral("PX4_SIM_MODEL=%1").arg(vehicleModel()),
        QStringLiteral("--security-opt=no-new-privileges"),
        QStringLiteral("--memory=512m"),
        QStringLiteral("--cpus=1.5"),
        QStringLiteral("--pids-limit=256"),
        QStringLiteral("--stop-timeout=10"),
        containerImage(),
    });

    docker.start();
    if (!docker.waitForFinished(30000)) {
        qCWarning(SITLTestLog) << "docker run timed out";
        return false;
    }

    if (docker.exitCode() != 0) {
        qCWarning(SITLTestLog) << "docker run failed:" << docker.readAllStandardError();
        return false;
    }

    _containerId = QString::fromUtf8(docker.readAllStandardOutput()).trimmed();
    qCInfo(SITLTestLog) << "Started SITL container:" << _containerId.left(12)
                        << "image:" << containerImage()
                        << "model:" << vehicleModel();
    return true;
}

bool SITLTestBase::stopContainer()
{
    if (_containerId.isEmpty()) {
        return true;
    }

    QProcess docker;
    docker.setProgram(QStringLiteral("docker"));
    docker.setArguments({QStringLiteral("stop"), _containerId});
    docker.start();
    docker.waitForFinished(15000);

    const bool ok = docker.exitCode() == 0;
    if (!ok) {
        qCWarning(SITLTestLog) << "docker stop failed:" << docker.readAllStandardError();
        // Force kill as fallback
        QProcess::execute(QStringLiteral("docker"), {QStringLiteral("kill"), _containerId});
    }

    _containerId.clear();
    return ok;
}

void SITLTestBase::captureContainerLogs()
{
    if (_containerId.isEmpty()) {
        return;
    }

    QProcess docker;
    docker.setProgram(QStringLiteral("docker"));
    docker.setArguments({QStringLiteral("logs"), _containerId});
    docker.start();
    docker.waitForFinished(10000);

    const QByteArray logs = docker.readAllStandardOutput() + docker.readAllStandardError();
    if (logs.isEmpty()) {
        return;
    }

    // Write to build directory for CI artifact upload
    const QString logDir = QDir(qEnvironmentVariable("CMAKE_BINARY_DIR",
                                    QStandardPaths::writableLocation(QStandardPaths::TempLocation)))
                               .filePath(QStringLiteral("sitl-logs"));
    QDir().mkpath(logDir);

    const QString logFile = QDir(logDir).filePath(
        QStringLiteral("px4-sitl-%1.log").arg(QTest::currentTestFunction()));
    QFile file(logFile);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(logs);
        qCInfo(SITLTestLog) << "PX4 logs saved to:" << logFile;
    }
}

// ----------------------------------------------------------------------------
// Connection lifecycle
// ----------------------------------------------------------------------------

bool SITLTestBase::connectToSITL()
{
    // Auto-connect is disabled in unit tests (LinkManager::init skips the timer).
    // We need to manually create a UDP link listening on the MAVLink port.
    // With --network host, PX4 broadcasts to 127.0.0.1:14550 on the host
    // network directly, so we bind to that port to receive heartbeats.
    auto *config = new UDPConfiguration(QStringLiteral("SITL Test Link"));
    config->setLocalPort(static_cast<quint16>(mavlinkPort()));
    config->setDynamic(true);

    auto sharedConfig = LinkManager::instance()->addConfiguration(config);
    if (!LinkManager::instance()->createConnectedLink(sharedConfig)) {
        qCWarning(SITLTestLog) << "Failed to create connected UDP link on port" << mavlinkPort();
        return false;
    }

    qCInfo(SITLTestLog) << "UDP link listening on port" << mavlinkPort();
    return true;
}

void SITLTestBase::disconnectFromSITL()
{
    // Disconnect all links and wait for everything to settle.
    // This must complete while QGCApplication singletons are still alive.
    LinkManager *lm = LinkManager::instance();
    if (!lm) {
        _vehicle = nullptr;
        return;
    }

    QSignalSpy spy(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);

    lm->disconnectAll();

    // Wait for vehicle removal if one exists
    if (_vehicle && MultiVehicleManager::instance()->activeVehicle()) {
        waitForSignal(spy, TestTimeout::longMs(), QStringLiteral("activeVehicleChanged (disconnect)"));
    }

    _vehicle = nullptr;

    // Give the event loop time to fully process link destruction
    settleEventLoopForCleanup(5, 200);
}

bool SITLTestBase::waitForVehicle(int timeoutMs)
{
    // Check if vehicle already appeared
    _vehicle = MultiVehicleManager::instance()->activeVehicle();
    if (_vehicle) {
        return true;
    }

    QSignalSpy spy(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
    if (!waitForSignal(spy, timeoutMs, QStringLiteral("activeVehicleChanged (SITL heartbeat)"))) {
        qCWarning(SITLTestLog) << "No heartbeat received from SITL within" << timeoutMs << "ms";
        return false;
    }

    _vehicle = MultiVehicleManager::instance()->activeVehicle();
    return _vehicle != nullptr;
}

bool SITLTestBase::waitForInitialConnect(int timeoutMs)
{
    if (!_vehicle) {
        return false;
    }

    if (_vehicle->isInitialConnectComplete()) {
        return true;
    }

    QSignalSpy spy(_vehicle, &Vehicle::initialConnectComplete);
    if (!waitForSignal(spy, timeoutMs, QStringLiteral("Vehicle::initialConnectComplete"))) {
        qCWarning(SITLTestLog) << "Initial connect sequence did not complete within" << timeoutMs << "ms";
        return false;
    }

    return true;
}

// ----------------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------------

QString SITLTestBase::_readDigestFile()
{
    // Look for digest file relative to source tree
    const QString digestPath = QStringLiteral(":/test/SITL/.github/px4-sitl-digest.txt");

    // Try common locations
    for (const QString &candidate : {
             QStringLiteral(".github/px4-sitl-digest.txt"),
             QDir(qEnvironmentVariable("GITHUB_WORKSPACE")).filePath(QStringLiteral(".github/px4-sitl-digest.txt")),
         }) {
        QFile file(candidate);
        if (file.open(QIODevice::ReadOnly)) {
            return QString::fromUtf8(file.readAll()).trimmed();
        }
    }

    return {};
}
