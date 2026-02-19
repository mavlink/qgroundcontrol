#pragma once

#include "UnitTest.h"

/// Unit tests for TestFixtures to ensure the test infrastructure itself is reliable
class TestFixturesTest : public UnitTest
{
    Q_OBJECT

public:
    TestFixturesTest() = default;

private slots:
    // Coordinate Fixtures
    void _testCoordOrigin();
    void _testCoordStandardLocations();
    void _testCoordEdgeCases();
    void _testCoordPolygon();
    void _testCoordWaypointPath();

    // TempFile/TempDir Fixtures
    void _testTempFileFixture();
    void _testTempFileFixtureWithTemplate();
    void _testTempJsonFileFixture();
    void _testTempDirFixture();
    void _testTempDirFixtureCreateFile();
    void _testNetworkReplyFixture();
    void _testSingleInstanceLockFixture();

    // SignalSpyFixture
    void _testSignalSpyFixtureExpect();
    void _testSignalSpyFixtureExpectExactly();
    void _testSignalSpyFixtureExpectNot();
    void _testSignalSpyFixtureWaitAndVerify();
    void _testWaitForSignalCountHelper();

    // SettingsFixture
    void _testSettingsFixtureRestore();
    void _testSettingsFixtureFactValue();
};
