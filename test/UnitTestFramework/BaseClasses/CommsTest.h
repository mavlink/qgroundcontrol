#pragma once

#include "MAVLinkLib.h"
#include "MockLink.h"
#include "UnitTest.h"

class LinkManager;
class Vehicle;

/// @file
/// @brief Base class for communication/link tests

/// Test fixture for communication and link management tests.
/// Does not automatically connect a vehicle - allows testing link
/// setup, configuration, and connection management.
///
/// Example usage:
/// @code
/// class MyCommsTest : public CommsTest
/// {
///     Q_OBJECT
/// private slots:
///     void _testLinkCreation() {
///         auto link = createMockLink();
///         QVERIFY(link);
///     }
/// };
/// @endcode
class CommsTest : public UnitTest
{
    Q_OBJECT

public:
    explicit CommsTest(QObject* parent = nullptr);
    ~CommsTest() override = default;

    /// Returns the link manager
    LinkManager* linkManager() const;

protected slots:
    void init() override;
    void cleanup() override;

protected:
    /// Creates a MockLink with default configuration
    SharedLinkInterfacePtr createMockLink(const QString& name = "TestLink", bool highLatency = false);

    /// Creates a MockLink and waits for vehicle connection
    Vehicle* createMockLinkAndWaitForVehicle(const QString& name = "TestLink",
                                             MAV_AUTOPILOT autopilot = MAV_AUTOPILOT_PX4);

    /// Disconnects all links
    void disconnectAllLinks();

    /// Waits for a vehicle to connect
    Vehicle* waitForVehicleConnect(int timeoutMs = 0);

    /// Waits for all vehicles to disconnect
    bool waitForAllVehiclesDisconnect(int timeoutMs = 0);

private:
    QList<SharedLinkInterfacePtr> _createdLinks;
};
