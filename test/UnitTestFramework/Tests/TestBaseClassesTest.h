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
        return _initCalled;
    }

    bool wasCleanupCalled() const
    {
        return _cleanupCalled;
    }

    bool hadVehicle() const
    {
        return _hadVehicle;
    }

    // Public wrappers for testing
    void doInit()
    {
        UnitTest::initTestCase();
        init();
    }

    void doCleanup()
    {
        cleanup();
        UnitTest::cleanupTestCase();
    }

public slots:

    void init() override
    {
        VehicleTest::init();
        _initCalled = true;
        _hadVehicle = (vehicle() != nullptr);
    }

    void cleanup() override
    {
        _cleanupCalled = true;
        VehicleTest::cleanup();
    }

private slots:

    void _dummyTest()
    {
        // Just a placeholder test method
    }

private:
    bool _initCalled = false;
    bool _cleanupCalled = false;
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

    // Public wrappers for testing
    void doInit()
    {
        UnitTest::initTestCase();
        init();
    }

    void doCleanup()
    {
        cleanup();
        UnitTest::cleanupTestCase();
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

private slots:

    void _dummyTest()
    {
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

    // Public wrappers for testing - expose protected methods
    void doInit()
    {
        UnitTest::initTestCase();
        init();
    }

    void doCleanup()
    {
        cleanup();
        UnitTest::cleanupTestCase();
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

private slots:

    void _dummyTest()
    {
    }
};
