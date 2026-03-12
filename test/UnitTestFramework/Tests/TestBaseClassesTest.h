#pragma once

#include "BaseClasses/TestBaseClasses.h"

/// Tests for the specialized test base classes
class TestBaseClassesTest : public UnitTest
{
    Q_OBJECT

public:
    TestBaseClassesTest() = default;

private slots:
    // We can't easily test VehicleTest etc. as unit tests since they ARE base classes.
    // Instead, we create simple derived classes inline to verify they work.

    void _testVehicleTestDerived();
    void _testOfflineMissionTestDerived();
    void _testCommsTestDerived();
};

/// Simple derived class to test VehicleTest base
class SimpleVehicleTest : public VehicleTest
{
    Q_OBJECT
    friend class TestBaseClassesTest;

public:
    SimpleVehicleTest() = default;

    bool wasInitCalled() const
    {
        return _initWasCalled;
    }

    bool wasCleanupCalled() const
    {
        return _cleanupWasCalled;
    }

    bool hadVehicle() const
    {
        return _hadVehicle;
    }

    void doInit()
    {
        init();
    }

    void doCleanup()
    {
        cleanup();
    }

public slots:

    void init() override
    {
        VehicleTest::init();
        _initWasCalled = true;
        _hadVehicle = (vehicle() != nullptr);
    }

    void cleanup() override
    {
        _cleanupWasCalled = true;
        VehicleTest::cleanup();
    }

private:
    bool _initWasCalled = false;
    bool _cleanupWasCalled = false;
    bool _hadVehicle = false;
};

/// Simple derived class to test OfflineMissionTest base
class SimpleOfflineMissionTest : public OfflineMissionTest
{
    Q_OBJECT
    friend class TestBaseClassesTest;

public:
    SimpleOfflineMissionTest() = default;

    bool hasMissionController() const
    {
        return missionController() != nullptr;
    }

    void doInit()
    {
        init();
    }

    void doCleanup()
    {
        cleanup();
    }

public slots:

    void init() override
    {
        OfflineMissionTest::init();
    }

    void cleanup() override
    {
        OfflineMissionTest::cleanup();
    }
};

/// Simple derived class to test CommsTest base
class SimpleCommsTest : public CommsTest
{
    Q_OBJECT
    friend class TestBaseClassesTest;

public:
    SimpleCommsTest() = default;

    bool hasLinkManager() const
    {
        return linkManager() != nullptr;
    }

    void doInit()
    {
        init();
    }

    void doCleanup()
    {
        cleanup();
    }

    SharedLinkInterfacePtr doCreateMockLink(const QString& name)
    {
        return createMockLink(name);
    }

    Vehicle* doWaitForVehicleConnect(int timeoutMs)
    {
        return waitForVehicleConnect(timeoutMs);
    }

public slots:

    void init() override
    {
        CommsTest::init();
    }

    void cleanup() override
    {
        CommsTest::cleanup();
    }
};
