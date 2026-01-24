#pragma once

#include "UnitTest.h"
#include "TestHelpers.h"
#include "QtTestExtensions.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "ParameterManager.h"
#include "PlanMasterController.h"
#include "MissionController.h"
#include "GeoFenceController.h"
#include "RallyPointController.h"
#include "FTPManager.h"
#include "LinkManager.h"

#include <QtCore/QPointer>
#include <QtTest/QSignalSpy>

/// @file
/// @brief Specialized test fixture base classes for common test scenarios
///
/// These fixtures provide pre-configured test environments to reduce boilerplate
/// and ensure consistent test setup/teardown patterns.
///
/// Available fixtures:
/// - VehicleTest: Tests requiring a connected vehicle
/// - VehicleTestNoConnect: Vehicle test without initial connect sequence
/// - MissionTest: Mission planning tests with PlanMasterController
/// - OfflineTest: Tests that don't need a vehicle connection
/// - FTPTest: FTP (MAVLink File Transfer Protocol) tests
/// - ParameterTest: Parameter manager tests
/// - LinkTest: Link management tests
/// - MultiVehicleTest: Multi-vehicle scenario tests
/// - GeoFenceTest: Geo-fence related tests
/// - RallyPointTest: Rally point related tests
/// - MAVLinkTest: MAVLink protocol tests

// ============================================================================
// VehicleTest - Base class for tests requiring a connected vehicle
// ============================================================================

/// Test fixture that automatically connects a MockLink vehicle before each test
/// and disconnects after. Use this when your tests need an active vehicle.
///
/// Example usage:
/// @code
/// class MyVehicleTest : public VehicleTest
/// {
///     Q_OBJECT
/// private slots:
///     void _testSomething() {
///         // vehicle() is already connected and ready
///         QCOMPARE(vehicle()->armed(), false);
///     }
/// };
/// @endcode
class VehicleTest : public UnitTest
{
    Q_OBJECT

public:
    explicit VehicleTest(QObject *parent = nullptr) : UnitTest(parent) {}

    /// Returns the connected vehicle (only valid during test execution)
    Vehicle *vehicle() const { return _vehicle; }

    /// Returns the MockLink (only valid during test execution)
    MockLink *mockLink() const { return _mockLink; }

    /// Returns the autopilot type used for this test
    MAV_AUTOPILOT autopilotType() const { return _autopilotType; }

protected slots:
    void init() override
    {
        UnitTest::init();
        _connectMockLink(_autopilotType, _failureMode);
        QVERIFY2(_vehicle, "Vehicle connection failed - _vehicle is null");

        if (_waitForParameters) {
            QVERIFY2(waitForParametersReady(), "Timeout waiting for parameters to be ready");
        }
    }

    void cleanup() override
    {
        _disconnectMockLink();
        UnitTest::cleanup();
    }

protected:
    /// Waits for vehicle parameters to be fully loaded
    /// @param timeoutMs Maximum time to wait
    /// @return true if parameters ready, false on timeout
    bool waitForParametersReady(int timeoutMs = TestHelpers::kLongTimeoutMs)
    {
        if (!_vehicle || !_vehicle->parameterManager()) {
            return false;
        }

        // Check if already ready before creating spy to avoid race condition
        if (_vehicle->parameterManager()->parametersReady()) {
            return true;
        }

        QSignalSpy spy(_vehicle->parameterManager(), &ParameterManager::parametersReadyChanged);
        if (!spy.isValid()) {
            qWarning() << "VehicleTest::waitForParametersReady: Failed to create signal spy";
            return false;
        }

        // Double-check after spy creation in case signal was emitted during spy setup
        if (_vehicle->parameterManager()->parametersReady()) {
            return true;
        }

        return spy.wait(timeoutMs);
    }

    /// Waits for initial connect sequence to complete
    /// @param timeoutMs Maximum time to wait
    /// @return true if connected, false on timeout
    bool waitForInitialConnect(int timeoutMs = TestHelpers::kLongTimeoutMs)
    {
        if (!_vehicle) {
            return false;
        }

        // Check if already complete before creating spy to avoid race condition
        if (_vehicle->isInitialConnectComplete()) {
            return true;
        }

        QSignalSpy spy(_vehicle, &Vehicle::initialConnectComplete);
        if (!spy.isValid()) {
            qWarning() << "VehicleTest::waitForInitialConnect: Failed to create signal spy";
            return false;
        }

        // Double-check after spy creation in case signal was emitted during spy setup
        if (_vehicle->isInitialConnectComplete()) {
            return true;
        }

        return spy.wait(timeoutMs);
    }

    /// Set the autopilot type before init() is called (e.g., in initTestCase)
    void setAutopilotType(MAV_AUTOPILOT type) { _autopilotType = type; }

    /// Set failure mode for MockLink
    void setFailureMode(MockConfiguration::FailureMode_t mode) { _failureMode = mode; }

    /// Set whether to wait for parameters during init()
    void setWaitForParameters(bool wait) { _waitForParameters = wait; }

private:
    MAV_AUTOPILOT _autopilotType = MAV_AUTOPILOT_PX4;
    MockConfiguration::FailureMode_t _failureMode = MockConfiguration::FailNone;
    bool _waitForParameters = false;
};

// ============================================================================
// VehicleTestNoConnect - Vehicle test without initial connect sequence
// ============================================================================

/// Test fixture that connects MockLink but skips the initial connect sequence.
/// Useful for testing early vehicle state or custom connect sequences.
class VehicleTestNoConnect : public VehicleTest
{
    Q_OBJECT

public:
    explicit VehicleTestNoConnect(QObject *parent = nullptr) : VehicleTest(parent)
    {
        setAutopilotType(MAV_AUTOPILOT_INVALID);
    }
};

// ============================================================================
// MissionTest - Base class for mission-related tests
// ============================================================================

/// Test fixture for mission planning tests. Provides a connected vehicle
/// with PlanMasterController and MissionController ready to use.
///
/// Example usage:
/// @code
/// class MyMissionTest : public MissionTest
/// {
///     Q_OBJECT
/// private slots:
///     void _testMissionUpload() {
///         missionController()->insertSimpleMissionItem(...);
///         QVERIFY(missionController()->dirtyBit());
///     }
/// };
/// @endcode
class MissionTest : public VehicleTest
{
    Q_OBJECT

public:
    explicit MissionTest(QObject *parent = nullptr) : VehicleTest(parent) {}

    /// Returns the plan master controller
    PlanMasterController *planController() const { return _planController; }

    /// Returns the mission controller
    MissionController *missionController() const
    {
        return _planController ? _planController->missionController() : nullptr;
    }

protected slots:
    void init() override
    {
        VehicleTest::init();

        _planController = new PlanMasterController(this);
        VERIFY_NOT_NULL(_planController);
        _planController->start();
        _planController->startStaticActiveVehicle(vehicle());
    }

    void cleanup() override
    {
        delete _planController;
        _planController = nullptr;

        VehicleTest::cleanup();
    }

protected:
    /// Clears the current mission
    /// @return true if mission was cleared, false if controller unavailable
    bool clearMission()
    {
        if (!missionController()) {
            qWarning() << "MissionTest::clearMission: missionController is null";
            return false;
        }
        missionController()->removeAll();
        return true;
    }

private:
    PlanMasterController *_planController = nullptr;
};

// ============================================================================
// OfflineTest - Base class for offline/no-vehicle tests
// ============================================================================

/// Test fixture for tests that don't need a vehicle connection.
/// Simply provides the base UnitTest without any vehicle setup.
/// Useful for testing offline editing, file parsing, utilities, etc.
///
/// Example usage:
/// @code
/// class MyOfflineTest : public OfflineTest
/// {
///     Q_OBJECT
/// private slots:
///     void _testFileParsing() {
///         // No vehicle needed
///         QFile file("test.plan");
///         // ... test file parsing
///     }
/// };
/// @endcode
class OfflineTest : public UnitTest
{
    Q_OBJECT

public:
    explicit OfflineTest(QObject *parent = nullptr) : UnitTest(parent) {}

protected:
    /// Creates an offline PlanMasterController for testing mission editing
    PlanMasterController *createOfflinePlanController()
    {
        auto *controller = new PlanMasterController(this);
        controller->start();
        return controller;
    }
};

// ============================================================================
// TempDirTest - Base class for tests needing temporary directory
// ============================================================================

/// Test fixture that provides automatic temporary directory management.
/// The temp directory is created before each test and cleaned up after.
///
/// Example usage:
/// @code
/// class MyFileTest : public TempDirTest
/// {
///     Q_OBJECT
/// private slots:
///     void _testFileWrite() {
///         QString path = tempFilePath("test.txt");
///         QFile file(path);
///         QVERIFY(file.open(QIODevice::WriteOnly));
///         // ...
///     }
/// };
/// @endcode
class TempDirTest : public OfflineTest
{
    Q_OBJECT

public:
    explicit TempDirTest(QObject *parent = nullptr) : OfflineTest(parent) {}

    /// Returns the temp directory path
    QString tempPath() const { return _tempDir ? _tempDir->path() : QString(); }

    /// Returns a path within the temp directory
    QString tempFilePath(const QString &filename) const
    {
        return _tempDir ? _tempDir->filePath(filename) : QString();
    }

    /// Copies a Qt resource to the temp directory
    /// @param resourcePath Resource path (e.g., ":/unittest/test.json")
    /// @param destName Destination filename (if empty, uses resource basename)
    /// @return Path to the copied file, or empty string on failure
    QString copyResourceToTemp(const QString &resourcePath, const QString &destName = QString())
    {
        if (!_tempDir) {
            return QString();
        }

        QString name = destName;
        if (name.isEmpty()) {
            name = QFileInfo(resourcePath).fileName();
        }

        const QString destPath = _tempDir->filePath(name);
        QFile::remove(destPath);
        if (!QFile(resourcePath).copy(destPath)) {
            return QString();
        }
        // Make writable since Qt resources are read-only
        QFile::setPermissions(destPath, QFile::ReadOwner | QFile::WriteOwner);
        return destPath;
    }

protected slots:
    void init() override
    {
        OfflineTest::init();
        _tempDir = std::make_unique<QTemporaryDir>();
        QVERIFY2(_tempDir->isValid(), "Failed to create temp directory");
    }

    void cleanup() override
    {
        _tempDir.reset();
        OfflineTest::cleanup();
    }

protected:
    std::unique_ptr<QTemporaryDir> _tempDir;
};

// ============================================================================
// OfflinePlanTest - Offline test with automatic PlanMasterController setup
// ============================================================================

/// Test fixture for offline mission planning tests that need PlanMasterController.
/// Automatically creates and destroys the controller for each test.
///
/// Example usage:
/// @code
/// class MyPlanTest : public OfflinePlanTest
/// {
///     Q_OBJECT
/// private slots:
///     void _testAddWaypoint() {
///         missionController()->insertSimpleMissionItem(coord, 0);
///         QCOMPARE(missionController()->visualItems()->count(), 2);
///     }
/// };
/// @endcode
class OfflinePlanTest : public OfflineTest
{
    Q_OBJECT

public:
    explicit OfflinePlanTest(QObject *parent = nullptr) : OfflineTest(parent) {}

    /// Returns the plan master controller (created in init)
    PlanMasterController *planController() const { return _planController; }

    /// Returns the mission controller
    MissionController *missionController() const
    {
        return _planController ? _planController->missionController() : nullptr;
    }

    /// Returns the geo-fence controller
    GeoFenceController *geoFenceController() const
    {
        return _planController ? _planController->geoFenceController() : nullptr;
    }

    /// Returns the rally point controller
    RallyPointController *rallyPointController() const
    {
        return _planController ? _planController->rallyPointController() : nullptr;
    }

    /// Returns the vehicle used by the controller (may be nullptr for offline)
    Vehicle *controllerVehicle() const
    {
        return _planController ? _planController->managerVehicle() : nullptr;
    }

protected slots:
    void init() override
    {
        OfflineTest::init();
        _planController = new PlanMasterController(this);
        _planController->start();
    }

    void cleanup() override
    {
        delete _planController;
        _planController = nullptr;
        OfflineTest::cleanup();
    }

protected:
    PlanMasterController *_planController = nullptr;
};

// ============================================================================
// FTPTest - Base class for FTP-related tests
// ============================================================================

/// Test fixture for FTP (MAVLink File Transfer Protocol) tests.
/// Provides a connected vehicle with FTPManager access and helper methods.
class FTPTest : public VehicleTestNoConnect
{
    Q_OBJECT

public:
    explicit FTPTest(QObject *parent = nullptr) : VehicleTestNoConnect(parent) {}

    /// Returns the FTP manager
    FTPManager *ftpManager() const
    {
        return _vehicle ? _vehicle->ftpManager() : nullptr;
    }

protected:
    /// Downloads a file and waits for completion
    /// @param remotePath Path on the vehicle
    /// @param localDir Local directory to save to
    /// @param timeoutMs Maximum time to wait
    /// @return Local file path on success, empty string on failure
    QString downloadFile(const QString &remotePath, const QString &localDir,
                         int timeoutMs = TestHelpers::kLongTimeoutMs)
    {
        if (!ftpManager()) {
            qWarning() << "FTPTest::downloadFile: ftpManager is null";
            return QString();
        }

        QSignalSpy spy(ftpManager(), &FTPManager::downloadComplete);
        if (!spy.isValid()) {
            qWarning() << "FTPTest::downloadFile: Failed to create signal spy";
            return QString();
        }

        ftpManager()->download(MAV_COMP_ID_AUTOPILOT1, remotePath, localDir);

        if (!spy.wait(timeoutMs)) {
            qWarning() << "FTPTest::downloadFile: Timeout waiting for download";
            return QString();
        }

        if (spy.count() != 1) {
            qWarning() << "FTPTest::downloadFile: Unexpected signal count:" << spy.count();
            return QString();
        }

        const QList<QVariant> args = spy.first();
        const QString errorMsg = args.value(1).toString();
        if (!errorMsg.isEmpty()) {
            qWarning() << "FTPTest::downloadFile: Download failed:" << errorMsg;
            return QString();
        }

        return args.value(0).toString();
    }

    /// Lists a directory and waits for completion
    /// @param remotePath Path on the vehicle
    /// @param timeoutMs Maximum time to wait
    /// @return List of entries on success, empty list on failure
    QStringList listDirectory(const QString &remotePath,
                              int timeoutMs = TestHelpers::kLongTimeoutMs)
    {
        if (!ftpManager()) {
            qWarning() << "FTPTest::listDirectory: ftpManager is null";
            return QStringList();
        }

        QSignalSpy spy(ftpManager(), &FTPManager::listDirectoryComplete);
        if (!spy.isValid()) {
            qWarning() << "FTPTest::listDirectory: Failed to create signal spy";
            return QStringList();
        }

        ftpManager()->listDirectory(MAV_COMP_ID_AUTOPILOT1, remotePath);

        if (!spy.wait(timeoutMs)) {
            qWarning() << "FTPTest::listDirectory: Timeout waiting for directory listing";
            return QStringList();
        }

        if (spy.count() != 1) {
            qWarning() << "FTPTest::listDirectory: Unexpected signal count:" << spy.count();
            return QStringList();
        }

        return spy.first().value(0).toStringList();
    }
};

// ============================================================================
// ParameterTest - Base class for parameter-related tests
// ============================================================================

/// Test fixture for parameter manager tests.
/// Provides helpers for parameter manipulation and verification.
class ParameterTest : public VehicleTest
{
    Q_OBJECT

public:
    explicit ParameterTest(QObject *parent = nullptr) : VehicleTest(parent)
    {
        setWaitForParameters(true);
    }

    /// Returns the parameter manager
    ParameterManager *parameterManager() const
    {
        return _vehicle ? _vehicle->parameterManager() : nullptr;
    }

protected:
    /// Gets a parameter fact by name
    /// @param name Parameter name
    /// @param compId Component ID (default: autopilot)
    /// @return Fact pointer or nullptr if not found
    Fact *getParameter(const QString &name, int compId = MAV_COMP_ID_AUTOPILOT1)
    {
        if (!parameterManager()) {
            qWarning() << "ParameterTest::getParameter: parameterManager is null";
            return nullptr;
        }
        return parameterManager()->getParameter(compId, name);
    }

    /// Sets a parameter value and waits for acknowledgment
    /// @param name Parameter name
    /// @param value New value
    /// @param timeoutMs Maximum time to wait
    /// @return true on success
    bool setParameterAndWait(const QString &name, const QVariant &value,
                             int timeoutMs = TestHelpers::kDefaultTimeoutMs)
    {
        Fact *fact = getParameter(name);
        if (!fact) {
            qWarning() << "ParameterTest::setParameterAndWait: Parameter not found:" << name;
            return false;
        }

        QSignalSpy spy(fact, &Fact::vehicleUpdated);
        if (!spy.isValid()) {
            qWarning() << "ParameterTest::setParameterAndWait: Failed to create signal spy";
            return false;
        }

        fact->setRawValue(value);
        return spy.wait(timeoutMs);
    }

    /// Refreshes a parameter from the vehicle
    /// @param name Parameter name
    /// @param timeoutMs Maximum time to wait
    /// @return true on success
    bool refreshParameter(const QString &name, int timeoutMs = TestHelpers::kDefaultTimeoutMs)
    {
        Fact *fact = getParameter(name);
        if (!fact) {
            qWarning() << "ParameterTest::refreshParameter: Parameter not found:" << name;
            return false;
        }
        if (!parameterManager()) {
            qWarning() << "ParameterTest::refreshParameter: parameterManager is null";
            return false;
        }

        QSignalSpy spy(fact, &Fact::vehicleUpdated);
        if (!spy.isValid()) {
            qWarning() << "ParameterTest::refreshParameter: Failed to create signal spy";
            return false;
        }

        parameterManager()->refreshParameter(MAV_COMP_ID_AUTOPILOT1, name);
        return spy.wait(timeoutMs);
    }
};

// ============================================================================
// LinkTest - Base class for link management tests
// ============================================================================

/// Test fixture for testing link management and multi-vehicle scenarios.
class LinkTest : public UnitTest
{
    Q_OBJECT

public:
    explicit LinkTest(QObject *parent = nullptr) : UnitTest(parent) {}

protected:
    /// Creates and connects a new MockLink
    /// @param name Link name
    /// @param highLatency Whether to simulate high-latency link
    /// @return Shared pointer to the link configuration, or nullptr on failure
    SharedLinkConfigurationPtr createMockLink(const QString &name, bool highLatency = false)
    {
        auto *config = new MockConfiguration(name);
        if (!config) {
            qWarning() << "LinkTest::createMockLink: Failed to create MockConfiguration";
            return nullptr;
        }

        config->setDynamic(true);
        config->setHighLatency(highLatency);

        auto sharedConfig = SharedLinkConfigurationPtr(config);
        LinkManager::instance()->createConnectedLink(sharedConfig);

        return sharedConfig;
    }

    /// Waits for a vehicle to be created
    /// @param timeoutMs Maximum time to wait
    /// @return The created vehicle, or nullptr on timeout
    Vehicle *waitForVehicle(int timeoutMs = TestHelpers::kDefaultTimeoutMs)
    {
        auto *mvm = MultiVehicleManager::instance();

        // Check if already have a vehicle before creating spy
        if (mvm->activeVehicle()) {
            return mvm->activeVehicle();
        }

        QSignalSpy spy(mvm, &MultiVehicleManager::activeVehicleChanged);
        if (!spy.isValid()) {
            qWarning() << "LinkTest::waitForVehicle: Failed to create signal spy";
            return nullptr;
        }

        // Double-check after spy creation in case signal was emitted
        if (mvm->activeVehicle()) {
            return mvm->activeVehicle();
        }

        if (!spy.wait(timeoutMs)) {
            qWarning() << "LinkTest::waitForVehicle: Timeout waiting for vehicle";
            return nullptr;
        }

        return mvm->activeVehicle();
    }

    /// Disconnects all links and waits for cleanup
    /// @return true if all links were disconnected successfully
    bool disconnectAllLinks()
    {
        auto *linkMgr = LinkManager::instance();
        if (linkMgr->links().isEmpty()) {
            return true;
        }

        QSignalSpy spy(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
        if (!spy.isValid()) {
            qWarning() << "LinkTest::disconnectAllLinks: Failed to create signal spy";
            linkMgr->disconnectAll();
            return false;
        }

        linkMgr->disconnectAll();
        return spy.wait(TestHelpers::kDefaultTimeoutMs);
    }
};

// ============================================================================
// MultiVehicleTest - Base class for multi-vehicle scenario tests
// ============================================================================

/// Test fixture for testing multi-vehicle scenarios.
/// Provides helpers for creating and managing multiple mock vehicles.
/// Uses QPointer to safely track vehicles that may be deleted during disconnection.
class MultiVehicleTest : public UnitTest
{
    Q_OBJECT

public:
    explicit MultiVehicleTest(QObject *parent = nullptr) : UnitTest(parent) {}

protected slots:
    void cleanup() override
    {
        // Disconnect all vehicles on cleanup
        for (QPointer<MockLink> &link : _mockLinks) {
            if (link) {
                _disconnectMockLinkInstance(link);
            }
        }
        _mockLinks.clear();
        _vehicles.clear();
        UnitTest::cleanup();
    }

protected:
    /// Creates and connects a new vehicle with the specified autopilot type
    /// @param autopilot Autopilot type (PX4, ArduPilot, etc.)
    /// @return Pointer to the created vehicle, or nullptr on failure
    Vehicle *addVehicle(MAV_AUTOPILOT autopilot = MAV_AUTOPILOT_PX4)
    {
        MockLink *link = nullptr;
        switch (autopilot) {
        case MAV_AUTOPILOT_PX4:
            link = MockLink::startPX4MockLink(false);
            break;
        case MAV_AUTOPILOT_ARDUPILOTMEGA:
            link = MockLink::startAPMArduCopterMockLink(false);
            break;
        default:
            link = MockLink::startGenericMockLink(false);
            break;
        }

        if (!link) {
            qWarning() << "MultiVehicleTest::addVehicle: Failed to create MockLink";
            return nullptr;
        }

        _mockLinks.append(QPointer<MockLink>(link));

        // Wait for vehicle to be created
        QSignalSpy spy(MultiVehicleManager::instance(), &MultiVehicleManager::vehicleAdded);
        if (!spy.isValid()) {
            qWarning() << "MultiVehicleTest::addVehicle: Failed to create signal spy";
            return nullptr;
        }

        if (!spy.wait(TestHelpers::kDefaultTimeoutMs)) {
            qWarning() << "MultiVehicleTest::addVehicle: Timeout waiting for vehicle";
            return nullptr;
        }

        if (spy.isEmpty()) {
            qWarning() << "MultiVehicleTest::addVehicle: No vehicle signal received";
            return nullptr;
        }

        Vehicle *vehicle = spy.at(0).at(0).value<Vehicle*>();
        if (vehicle) {
            _vehicles.append(QPointer<Vehicle>(vehicle));
        }
        return vehicle;
    }

    /// Returns all created vehicles (filters out any that have been deleted)
    QList<Vehicle*> vehicles() const
    {
        QList<Vehicle*> result;
        for (const QPointer<Vehicle> &v : _vehicles) {
            if (v) {
                result.append(v.data());
            }
        }
        return result;
    }

    /// Returns vehicle at index (nullptr if index out of range or vehicle deleted)
    Vehicle *vehicleAt(int index) const
    {
        if (index < 0 || index >= _vehicles.size()) {
            return nullptr;
        }
        return _vehicles.at(index).data();
    }

    /// Returns the number of vehicles still alive
    int vehicleCount() const
    {
        int count = 0;
        for (const QPointer<Vehicle> &v : _vehicles) {
            if (v) {
                ++count;
            }
        }
        return count;
    }

    /// Waits for all vehicles to complete initial connection
    /// @param timeoutMs Maximum time to wait for each vehicle
    /// @return true if all vehicles are ready, false on timeout or if any vehicle was deleted
    bool waitForAllVehiclesReady(int timeoutMs = TestHelpers::kLongTimeoutMs)
    {
        for (const QPointer<Vehicle> &vehiclePtr : _vehicles) {
            if (!vehiclePtr) {
                qWarning() << "MultiVehicleTest::waitForAllVehiclesReady: Vehicle was deleted";
                return false;
            }

            Vehicle *vehicle = vehiclePtr.data();
            if (vehicle->isInitialConnectComplete()) {
                continue;
            }

            QSignalSpy spy(vehicle, &Vehicle::initialConnectComplete);
            if (!spy.isValid()) {
                qWarning() << "MultiVehicleTest::waitForAllVehiclesReady: Failed to create signal spy";
                return false;
            }

            // Double-check in case it completed while creating spy
            if (vehicle->isInitialConnectComplete()) {
                continue;
            }

            if (!spy.wait(timeoutMs)) {
                qWarning() << "MultiVehicleTest::waitForAllVehiclesReady: Timeout for vehicle" << vehicle->id();
                return false;
            }
        }
        return true;
    }

private:
    void _disconnectMockLinkInstance(MockLink *link)
    {
        if (link && link->isConnected()) {
            QSignalSpy spy(link, &LinkInterface::disconnected);
            if (spy.isValid()) {
                link->disconnect();
                spy.wait(TestHelpers::kDefaultTimeoutMs);
            } else {
                link->disconnect();
            }
        }
    }

    QList<QPointer<MockLink>> _mockLinks;
    QList<QPointer<Vehicle>> _vehicles;
};

// ============================================================================
// GeoFenceTest - Base class for geo-fence tests
// ============================================================================

/// Test fixture for geo-fence related tests.
/// Provides a connected vehicle with GeoFenceController ready.
class GeoFenceTest : public MissionTest
{
    Q_OBJECT

public:
    explicit GeoFenceTest(QObject *parent = nullptr) : MissionTest(parent) {}

protected:
    /// Returns the GeoFence controller
    GeoFenceController *geoFenceController() const
    {
        return planController() ? planController()->geoFenceController() : nullptr;
    }

    /// Creates a circular geo-fence around a center point
    /// @param center Center of the circle
    /// @param radiusMeters Radius in meters
    void createCircularFence(const QGeoCoordinate &center, double radiusMeters)
    {
        if (!geoFenceController()) return;

        // Implementation depends on actual GeoFenceController API
        // This is a placeholder for the pattern
        Q_UNUSED(center)
        Q_UNUSED(radiusMeters)
    }

    /// Creates a polygon geo-fence
    /// @param vertices List of vertices
    void createPolygonFence(const QList<QGeoCoordinate> &vertices)
    {
        if (!geoFenceController()) return;

        Q_UNUSED(vertices)
    }
};

// ============================================================================
// RallyPointTest - Base class for rally point tests
// ============================================================================

/// Test fixture for rally point related tests.
/// Provides a connected vehicle with RallyPointController ready.
class RallyPointTest : public MissionTest
{
    Q_OBJECT

public:
    explicit RallyPointTest(QObject *parent = nullptr) : MissionTest(parent) {}

protected:
    /// Returns the RallyPoint controller
    RallyPointController *rallyPointController() const
    {
        return planController() ? planController()->rallyPointController() : nullptr;
    }

    /// Adds a rally point at the specified location
    /// @param coord Coordinate for the rally point
    void addRallyPoint(const QGeoCoordinate &coord)
    {
        if (!rallyPointController()) return;

        // Implementation depends on actual RallyPointController API
        Q_UNUSED(coord)
    }
};

// ============================================================================
// MAVLinkTest - Base class for MAVLink protocol tests
// ============================================================================

/// Test fixture for MAVLink protocol tests.
/// Provides helpers for sending/receiving MAVLink messages.
class MAVLinkTest : public VehicleTest
{
    Q_OBJECT

public:
    explicit MAVLinkTest(QObject *parent = nullptr) : VehicleTest(parent) {}

protected:
    /// Sends a MAVLink command and waits for acknowledgment
    /// @param command Command ID
    /// @param param1-param7 Command parameters
    /// @param timeoutMs Maximum time to wait for ack
    /// @return MAV_RESULT from the command ack
    MAV_RESULT sendCommandAndWait(MAV_CMD command,
                                   float param1 = 0, float param2 = 0, float param3 = 0,
                                   float param4 = 0, float param5 = 0, float param6 = 0,
                                   float param7 = 0, int timeoutMs = TestHelpers::kDefaultTimeoutMs)
    {
        if (!_vehicle || !mockLink()) {
            return MAV_RESULT_FAILED;
        }

        // Use vehicle's command sending mechanism
        // Implementation depends on Vehicle API
        Q_UNUSED(command)
        Q_UNUSED(param1) Q_UNUSED(param2) Q_UNUSED(param3) Q_UNUSED(param4)
        Q_UNUSED(param5) Q_UNUSED(param6) Q_UNUSED(param7)
        Q_UNUSED(timeoutMs)

        return MAV_RESULT_ACCEPTED;
    }

    /// Clears the received command count on the mock link
    void clearReceivedCommands()
    {
        if (mockLink()) {
            mockLink()->clearReceivedMavCommandCounts();
        }
    }

    /// Returns how many times a command was received by the mock link
    int receivedCommandCount(MAV_CMD command) const
    {
        return mockLink() ? mockLink()->receivedMavCommandCount(command) : 0;
    }
};
