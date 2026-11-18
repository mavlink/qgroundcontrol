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
    void _testCompareCoords();

    // TempFile/TempDir Fixtures
    void _testTempFileFixture();
    void _testTempFileFixtureWithTemplate();
    void _testTempJsonFileFixture();
    void _testTempDirFixture();
    void _testTempDirFixtureCreateFile();
    void _testWriteFileIn();
    void _testCopyResource();
    void _testNetworkReplyFixture();

    // SettingsFixture
    void _testSettingsFixtureRestore();
    void _testSettingsFixtureFactValue();

    // LoggingCategoryFixture
    void _testLoggingCategoryFixtureRestore();
};
