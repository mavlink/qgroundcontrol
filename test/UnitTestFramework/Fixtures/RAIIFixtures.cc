#include "RAIIFixtures.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "AppSettings.h"
#include "Fact.h"
#include "LinkManager.h"
#include "MockLink.h"
#include "MultiVehicleManager.h"
#include <QtCore/QLoggingCategory>
#include "SettingsManager.h"
#include "Vehicle.h"

Q_STATIC_LOGGING_CATEGORY(RAIIFixturesLog, "Test.RAIIFixtures")

namespace TestFixtures {

// ============================================================================
// VehicleFixture Implementation
// ============================================================================

VehicleFixture::VehicleFixture(VehicleTest* test, MAV_AUTOPILOT autopilot, bool waitForInitialConnect) : _test(test)
{
    if (!_test) {
        qCWarning(RAIIFixturesLog) << "VehicleFixture: null VehicleTest";
        return;
    }

    QSignalSpy spyVehicle(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);

    // Create MockLink configuration
    MockConfiguration* mockConfig = new MockConfiguration(QStringLiteral("VehicleFixture"));
    mockConfig->setDynamic(true);
    mockConfig->setFirmwareType(autopilot);

    SharedLinkConfigurationPtr sharedConfig(mockConfig);

    if (!LinkManager::instance()->createConnectedLink(sharedConfig)) {
        qCWarning(RAIIFixturesLog) << "VehicleFixture: failed to create MockLink";
        return;
    }

    _mockLink = qobject_cast<MockLink*>(mockConfig->link());
    if (!_mockLink) {
        qCWarning(RAIIFixturesLog) << "VehicleFixture: link is not MockLink";
        return;
    }

    // Wait for vehicle to connect
    if (!spyVehicle.wait(TestTimeout::longMs())) {
        qCWarning(RAIIFixturesLog) << "VehicleFixture: timeout waiting for vehicle";
        return;
    }

    _vehicle = MultiVehicleManager::instance()->activeVehicle();
    if (!_vehicle) {
        qCWarning(RAIIFixturesLog) << "VehicleFixture: no active vehicle after connection";
        return;
    }

    // Wait for initial connect sequence if requested
    if (waitForInitialConnect && autopilot != MAV_AUTOPILOT_INVALID) {
        QSignalSpy spyConnect(_vehicle, &Vehicle::initialConnectComplete);
        if (!spyConnect.wait(TestTimeout::longMs())) {
            qCWarning(RAIIFixturesLog) << "VehicleFixture: timeout waiting for initialConnectComplete";
        }
    }
}

VehicleFixture::~VehicleFixture()
{
    if (_mockLink) {
        QSignalSpy spyVehicle(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);

        _mockLink->disconnect();

        // Wait for vehicle to disconnect
        if (!spyVehicle.wait(TestTimeout::longMs())) {
            qCWarning(RAIIFixturesLog) << "~VehicleFixture: timeout waiting for vehicle disconnect";
        }

        // Process pending events for cleanup
        for (int i = 0; i < 5; ++i) {
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            if (i < 4) {
                QTest::qWait(10);
            }
        }
    }
}

void VehicleFixture::setCommLost(bool lost)
{
    if (_mockLink) {
        _mockLink->setCommLost(lost);
    }
}

void VehicleFixture::simulateConnectionRemoved()
{
    if (_mockLink) {
        _mockLink->simulateConnectionRemoved();
    }
}

// ============================================================================
// SettingsFixture Implementation
// ============================================================================

SettingsFixture::SettingsFixture()
{
    AppSettings* appSettings = SettingsManager::instance()->appSettings();

    // Save original values
    _originalFirmware = static_cast<MAV_AUTOPILOT>(QGCMAVLink::firmwareClassToAutopilot(
        static_cast<QGCMAVLink::FirmwareClass_t>(appSettings->offlineEditingFirmwareClass()->rawValue().toInt())));

    _originalVehicleType = static_cast<MAV_TYPE>(QGCMAVLink::vehicleClassToMavType(
        static_cast<QGCMAVLink::VehicleClass_t>(appSettings->offlineEditingVehicleClass()->rawValue().toInt())));
}

SettingsFixture::~SettingsFixture()
{
    // Restore original settings
    AppSettings* appSettings = SettingsManager::instance()->appSettings();

    appSettings->offlineEditingFirmwareClass()->setRawValue(QGCMAVLink::firmwareClass(_originalFirmware));

    appSettings->offlineEditingVehicleClass()->setRawValue(QGCMAVLink::vehicleClass(_originalVehicleType));

    // Restore any saved Fact values
    for (const SavedFact& saved : _savedFacts) {
        if (saved.fact) {
            saved.fact->setRawValue(saved.originalValue);
        }
    }
}

void SettingsFixture::setOfflineFirmware(MAV_AUTOPILOT autopilot)
{
    AppSettings* appSettings = SettingsManager::instance()->appSettings();
    appSettings->offlineEditingFirmwareClass()->setRawValue(QGCMAVLink::firmwareClass(autopilot));
}

void SettingsFixture::setOfflineVehicleType(MAV_TYPE vehicleType)
{
    AppSettings* appSettings = SettingsManager::instance()->appSettings();
    appSettings->offlineEditingVehicleClass()->setRawValue(QGCMAVLink::vehicleClass(vehicleType));
}

void SettingsFixture::setAltitudeMode(int altitudeMode)
{
    Q_UNUSED(altitudeMode);
}

void SettingsFixture::setFactValue(Fact* fact, const QVariant& value)
{
    if (!fact) {
        qCWarning(RAIIFixturesLog) << "setFactValue: null Fact";
        return;
    }

    // Save original value for restoration
    _savedFacts.append({fact, fact->rawValue()});

    // Set new value
    fact->setRawValue(value);
}

// ============================================================================
// SignalSpyFixture Implementation
// ============================================================================

SignalSpyFixture::SignalSpyFixture(QObject* target) : _target(target), _spy(std::make_unique<MultiSignalSpy>())
{
    if (_target) {
        _spy->init(_target);
    } else {
        qCWarning(RAIIFixturesLog) << "SignalSpyFixture: null target";
    }
}

SignalSpyFixture::~SignalSpyFixture() = default;

void SignalSpyFixture::expect(const char* signalName)
{
    _expectations.append({QString::fromLatin1(signalName), -1});  // -1 = at least once
}

void SignalSpyFixture::expectExactly(const char* signalName, int count)
{
    _expectations.append({QString::fromLatin1(signalName), count});
}

void SignalSpyFixture::expectNot(const char* signalName)
{
    _expectations.append({QString::fromLatin1(signalName), 0});  // 0 = never
}

void SignalSpyFixture::clear()
{
    _spy->clearAllSignals();
    _expectations.clear();
}

bool SignalSpyFixture::verify() const
{
    QString errorMsg;
    return verify(errorMsg);
}

bool SignalSpyFixture::verify(QString& errorMsg) const
{
    errorMsg.clear();

    for (const Expectation& exp : _expectations) {
        const int count = emissionCount(qPrintable(exp.signalName));

        if (exp.expectedCount == -1) {
            // At least once
            if (count < 1) {
                errorMsg = QStringLiteral("Expected signal '%1' to be emitted at least once, but was emitted %2 times")
                               .arg(exp.signalName)
                               .arg(count);
                return false;
            }
        } else if (exp.expectedCount == 0) {
            // Never
            if (count > 0) {
                errorMsg = QStringLiteral("Expected signal '%1' to NOT be emitted, but was emitted %2 times")
                               .arg(exp.signalName)
                               .arg(count);
                return false;
            }
        } else {
            // Exactly N times
            if (count != exp.expectedCount) {
                errorMsg = QStringLiteral("Expected signal '%1' to be emitted %2 times, but was emitted %3 times")
                               .arg(exp.signalName)
                               .arg(exp.expectedCount)
                               .arg(count);
                return false;
            }
        }
    }

    return true;
}

bool SignalSpyFixture::waitAndVerify(int timeoutMs)
{
    // Wait for signals with polling
    const int pollInterval = 50;
    int elapsed = 0;

    while (elapsed < timeoutMs) {
        QTest::qWait(pollInterval);
        elapsed += pollInterval;

        // Check if all expectations are met
        bool allMet = true;
        for (const Expectation& exp : _expectations) {
            const int count = emissionCount(qPrintable(exp.signalName));
            if (exp.expectedCount == -1 && count < 1) {
                allMet = false;
                break;
            } else if (exp.expectedCount > 0 && count < exp.expectedCount) {
                allMet = false;
                break;
            }
        }

        if (allMet) {
            return verify();
        }
    }

    return verify();
}

bool SignalSpyFixture::wasEmitted(const char* signalName) const
{
    return emissionCount(signalName) > 0;
}

int SignalSpyFixture::emissionCount(const char* signalName) const
{
    if (!_spy)
        return 0;

    // Try with SIGNAL() macro format first
    QString sigWithMacro = QStringLiteral("2") + QString::fromLatin1(signalName);
    if (!sigWithMacro.contains('(')) {
        sigWithMacro += QStringLiteral("()");
    }

    int result = _spy->count(qPrintable(sigWithMacro));
    if (result >= 0) {
        return result;
    }

    // Try without macro format
    return _spy->count(signalName);
}

// ============================================================================
// TempFileFixture Implementation
// ============================================================================

TempFileFixture::TempFileFixture(const QString& templateName)
{
    if (templateName.isEmpty()) {
        _file = std::make_unique<QTemporaryFile>();
    } else {
        _file = std::make_unique<QTemporaryFile>(QDir::tempPath() + "/" + templateName);
    }

    if (!_file->open()) {
        qCWarning(RAIIFixturesLog) << "TempFileFixture: failed to create temp file:" << _file->errorString();
        _file.reset();
    }
}

TempFileFixture::~TempFileFixture() = default;

QString TempFileFixture::path() const
{
    return _file ? _file->fileName() : QString();
}

bool TempFileFixture::write(const QByteArray& content)
{
    if (!_file)
        return false;

    _file->seek(0);
    _file->resize(0);
    return _file->write(content) == content.size();
}

bool TempFileFixture::write(const QString& content)
{
    return write(content.toUtf8());
}

QByteArray TempFileFixture::readAll()
{
    if (!_file)
        return QByteArray();

    _file->seek(0);
    return _file->readAll();
}

// ============================================================================
// TempDirFixture Implementation
// ============================================================================

TempDirFixture::TempDirFixture() : _dir(std::make_unique<QTemporaryDir>())
{
    if (!_dir->isValid()) {
        qCWarning(RAIIFixturesLog) << "TempDirFixture: failed to create temp directory";
        _dir.reset();
    }
}

TempDirFixture::~TempDirFixture() = default;

QString TempDirFixture::path() const
{
    return _dir ? _dir->path() : QString();
}

QString TempDirFixture::createFile(const QString& relativePath, const QByteArray& content)
{
    if (!_dir)
        return QString();

    const QString fullPath = _dir->path() + "/" + relativePath;

    // Create parent directories if needed
    const QFileInfo fileInfo(fullPath);
    QDir().mkpath(fileInfo.absolutePath());

    QFile file(fullPath);
    if (!file.open(QIODevice::WriteOnly)) {
        qCWarning(RAIIFixturesLog) << "TempDirFixture::createFile: failed to create file:" << fullPath;
        return QString();
    }

    if (!content.isEmpty()) {
        file.write(content);
    }

    return fullPath;
}

}  // namespace TestFixtures
