#pragma once

#include <QtCore/QList>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTemporaryFile>
#include <QtCore/QVariant>

#include <memory>

#include "BaseClasses/VehicleTest.h"
#include "MAVLinkLib.h"
#include "MultiSignalSpy.h"

class MockLink;
class Vehicle;
class Fact;

/// @file
/// @brief RAII wrappers for test resources

namespace TestFixtures {

// ============================================================================
// VehicleFixture - RAII wrapper for MockLink vehicle connection
// ============================================================================

/// RAII fixture that connects a MockLink vehicle and auto-disconnects on destruction
/// Usage:
///   VehicleFixture vehicle(this, MAV_AUTOPILOT_PX4);
///   QVERIFY(vehicle.isConnected());
///   vehicle->doSomething();  // Access via operator->
class VehicleFixture
{
public:
    /// Connect a MockLink vehicle
    /// @param test The VehicleTest instance (for access to _connectMockLink)
    /// @param autopilot Autopilot type to simulate
    /// @param waitForInitialConnect If true, wait for full initial connect sequence
    explicit VehicleFixture(VehicleTest* test, MAV_AUTOPILOT autopilot = MAV_AUTOPILOT_PX4,
                            bool waitForInitialConnect = true);

    /// Disconnects the MockLink automatically
    ~VehicleFixture();

    // Non-copyable
    VehicleFixture(const VehicleFixture&) = delete;
    VehicleFixture& operator=(const VehicleFixture&) = delete;

    /// Check if vehicle is connected
    bool isConnected() const
    {
        return _vehicle != nullptr;
    }

    /// Get the Vehicle pointer
    Vehicle* vehicle() const
    {
        return _vehicle;
    }

    /// Get the MockLink pointer
    MockLink* mockLink() const
    {
        return _mockLink;
    }

    /// Pointer-like access to Vehicle
    Vehicle* operator->() const
    {
        return _vehicle;
    }

    /// Simulate communication loss
    void setCommLost(bool lost);

    /// Simulate connection removed
    void simulateConnectionRemoved();

private:
    VehicleTest* _test = nullptr;
    Vehicle* _vehicle = nullptr;
    MockLink* _mockLink = nullptr;
};

// ============================================================================
// SettingsFixture - RAII wrapper for settings save/restore
// ============================================================================

/// RAII fixture that saves settings on construction and restores them on destruction
/// Prevents test settings from polluting subsequent tests
class SettingsFixture
{
public:
    /// Save current settings state
    SettingsFixture();

    /// Restore original settings
    ~SettingsFixture();

    // Non-copyable
    SettingsFixture(const SettingsFixture&) = delete;
    SettingsFixture& operator=(const SettingsFixture&) = delete;

    /// Set offline editing firmware type
    void setOfflineFirmware(MAV_AUTOPILOT autopilot);

    /// Set offline editing vehicle type
    void setOfflineVehicleType(MAV_TYPE vehicleType);

    /// Set global altitude mode
    void setAltitudeMode(int altitudeMode);

    /// Set a Fact value (will be restored on destruction)
    void setFactValue(Fact* fact, const QVariant& value);

private:
    struct SavedFact
    {
        Fact* fact;
        QVariant originalValue;
    };

    QList<SavedFact> _savedFacts;

    MAV_AUTOPILOT _originalFirmware;
    MAV_TYPE _originalVehicleType;
};

// ============================================================================
// SignalSpyFixture - Enhanced signal monitoring with expectations
// ============================================================================

/// Enhanced signal spy that tracks expected signals and provides verification
/// Usage:
///   SignalSpyFixture spy(myObject);
///   spy.expect("valueChanged");
///   spy.expect("stateChanged");
///   // ... do something that should emit signals ...
///   QVERIFY(spy.verify());
class SignalSpyFixture
{
public:
    /// Create a signal spy for the given object
    /// @param target Object to monitor for signals
    explicit SignalSpyFixture(QObject* target);

    ~SignalSpyFixture();

    // Non-copyable
    SignalSpyFixture(const SignalSpyFixture&) = delete;
    SignalSpyFixture& operator=(const SignalSpyFixture&) = delete;

    /// Expect a signal to be emitted (at least once)
    /// @param signalName Signal name without SIGNAL() macro
    void expect(const char* signalName);

    /// Expect a signal to be emitted exactly N times
    /// @param signalName Signal name
    /// @param count Expected emission count
    void expectExactly(const char* signalName, int count);

    /// Expect a signal to NOT be emitted
    /// @param signalName Signal name
    void expectNot(const char* signalName);

    /// Clear all signals and expectations
    void clear();

    /// Verify all expectations are met
    /// @return true if all expectations passed
    bool verify() const;

    /// Verify and return detailed error message if failed
    /// @param errorMsg Output: error message if verification fails
    /// @return true if all expectations passed
    bool verify(QString& errorMsg) const;

    /// Wait for expected signals with timeout
    /// @param timeoutMs Maximum time to wait
    /// @return true if all expected signals received within timeout
    bool waitAndVerify(int timeoutMs = TestTimeout::mediumMs());

    /// Get underlying MultiSignalSpy for advanced usage
    MultiSignalSpy* spy() const
    {
        return _spy.get();
    }

    /// Check if a specific signal was emitted
    bool wasEmitted(const char* signalName) const;

    /// Get emission count for a signal
    int emissionCount(const char* signalName) const;

private:
    struct Expectation
    {
        QString signalName;
        int expectedCount;  // -1 = at least once, 0 = never, >0 = exactly N times
    };

    QObject* _target = nullptr;
    std::unique_ptr<MultiSignalSpy> _spy;
    QList<Expectation> _expectations;
};

// ============================================================================
// TempFileFixture - RAII wrapper for temporary files
// ============================================================================

/// RAII fixture for temporary files that auto-delete on destruction
class TempFileFixture
{
public:
    /// Create a temporary file
    /// @param templateName Optional template (e.g., "test_XXXXXX.txt")
    explicit TempFileFixture(const QString& templateName = QString());

    ~TempFileFixture();

    // Non-copyable
    TempFileFixture(const TempFileFixture&) = delete;
    TempFileFixture& operator=(const TempFileFixture&) = delete;

    /// Check if file was created successfully
    bool isValid() const
    {
        return _file && _file->isOpen();
    }

    /// Get the file path
    QString path() const;

    /// Write content to the file
    bool write(const QByteArray& content);
    bool write(const QString& content);

    /// Read all content from the file
    QByteArray readAll();

    /// Get underlying QTemporaryFile
    QTemporaryFile* file() const
    {
        return _file.get();
    }

private:
    std::unique_ptr<QTemporaryFile> _file;
};

// ============================================================================
// TempDirFixture - RAII wrapper for temporary directories
// ============================================================================

/// RAII fixture for temporary directories that auto-delete on destruction
class TempDirFixture
{
public:
    TempDirFixture();
    ~TempDirFixture();

    // Non-copyable
    TempDirFixture(const TempDirFixture&) = delete;
    TempDirFixture& operator=(const TempDirFixture&) = delete;

    /// Check if directory was created successfully
    bool isValid() const
    {
        return _dir && _dir->isValid();
    }

    /// Get the directory path
    QString path() const;

    /// Create a file in the temp directory
    /// @param relativePath Relative path within temp directory
    /// @param content Optional content to write
    /// @return Full path to created file, or empty string on failure
    QString createFile(const QString& relativePath, const QByteArray& content = QByteArray());

private:
    std::unique_ptr<QTemporaryDir> _dir;
};

}  // namespace TestFixtures
