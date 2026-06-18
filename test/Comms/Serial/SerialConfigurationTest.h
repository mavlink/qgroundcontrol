#pragma once

#include "UnitTest.h"

class SerialConfigurationTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testPortConfigEnumMapping();
    void _testParityMigrationFromLegacy();
    void _testParityMigrationClampsOutOfRange();
    void _testParityV2TakesPrecedenceOverLegacy();
    void _testSettingsRoundtripWritesParityV2();
    void _testSupportedBaudRatesSortedUniqueNonEmpty();
    void _testCleanPortDisplayNameUnknownReturnsEmpty();
};
