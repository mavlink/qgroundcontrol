#pragma once

#include "Protocols/Support/GPSProtocolTestBase.h"

/// Checks the u-blox configuration keys the driver uses against the key database of the pinned pyubx2
/// (fixtures/UBXConfigKeyDatabase.h). The scripted receiver reads the driver's own key constants, so only an
/// independent table can tell that a key id or the size of its value is wrong.
class UBXConfigKeyTest : public GPSProtocolTestBase
{
    Q_OBJECT

private slots:
    void _valueSizes();
    void _namedKeys_data();
    void _namedKeys();
    void _plannedKeys();
    void _writtenKeys_data();
    void _writtenKeys();
};
