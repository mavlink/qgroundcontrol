#pragma once

#include "UnitTest.h"

/// The receiver's satellites in view as the rows of a list model.
class GPSSatelliteModelTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _rowsChangeInPlace();
    void _reportReplacesRows_data();
    void _reportReplacesRows();
};
