#pragma once

#include "UnitTest.h"

/// The mountpoint browser of the RTK Corrections page: what a row of its list says, where the distances are measured
/// from, and the name filter.
class NTRIPMountpointBrowserTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _rowShowsDistanceInTheDistanceUnit();
    void _rowSaysWhyAMountpointCannotWork();
    void _browseMeasuresFromTheGroundStation();
    void _filterNarrowsTheList();
};
