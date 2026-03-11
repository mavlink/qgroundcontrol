#pragma once

#include "BaseClasses/VehicleTestManualConnect.h"

/// Data-driven test matrix for the _HASH_CHECK parameter cache optimization.
/// See _hashCheckMatrix_data() for the full scenario table.
class HashCheckTest : public VehicleTestManualConnect
{
    Q_OBJECT

private slots:
    void cleanup() override;

    void _hashCheckMatrix_data();
    void _hashCheckMatrix();

private:
    void _deleteCacheFiles();
    void _connectAndWaitForParams();
    void _disconnectAndSettle();
    MockLink *_startPX4MockLinkNoIncrement(MockConfiguration::FailureMode_t failureMode = MockConfiguration::FailNone);
    MockLink *_startPX4MockLinkHighLatency();
};
