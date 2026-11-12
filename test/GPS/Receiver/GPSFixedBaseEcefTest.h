#pragma once

#include "UnitTest.h"

/// The fixed base position of the receiver settings shown and entered as ECEF coordinates.
class GPSFixedBaseEcefTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _followsSavedPosition();
    void _editSavesPosition();
    void _showingChangesNothing();
    void _savesOnlyNearEarthSurface();
};
