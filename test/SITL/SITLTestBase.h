/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "UnitTest.h"

class Vehicle;
class SharedLinkConfiguration;

/// Base class for SITL integration tests.
///
/// Manages the lifecycle of a PX4 SITL Docker container and creates a real
/// UDP MAVLink link to it. Each test method gets a fresh container (started
/// in init(), stopped in cleanup()) for complete isolation.
///
/// Subclasses override containerImage() and vehicleModel() to customize
/// the PX4 configuration. The base class handles container startup, readiness
/// detection (MAV_STATE_STANDBY heartbeat), and log capture on teardown.
class SITLTestBase : public UnitTest
{
    Q_OBJECT

public:
    explicit SITLTestBase(QObject *parent = nullptr);
    ~SITLTestBase() override = default;

protected slots:
    void init() override;
    void cleanup() override;

protected:
    // Override in subclasses to customize container configuration
    virtual QString containerImage() const;
    virtual QString vehicleModel() const { return QStringLiteral("sihsim_quadx"); }
    virtual int mavlinkPort() const { return 14550; }
    virtual int readinessTimeoutMs() const { return 30000; }
    virtual int initialConnectTimeoutMs() const { return 60000; }

    // Container lifecycle
    bool startContainer();
    bool stopContainer();
    void captureContainerLogs();

    // Connection lifecycle
    bool connectToSITL();
    void disconnectFromSITL();
    bool waitForVehicle(int timeoutMs);
    bool waitForInitialConnect(int timeoutMs);

    // Accessors
    Vehicle *vehicle() const { return _vehicle; }
    QString containerId() const { return _containerId; }

    Vehicle *_vehicle = nullptr;
    QString _containerId;

private:
    static QString _readDigestFile();
};

