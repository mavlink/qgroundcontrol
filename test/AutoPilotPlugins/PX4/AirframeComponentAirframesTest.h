#pragma once

#include "UnitTest.h"

/// Tests for the airframe type list built by PX4AirframeLoader into
/// AirframeComponentAirframes, in particular the display ordering of the
/// airframe groups.
class AirframeComponentAirframesTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _sortedTypesStandardFramesFirst();
};
