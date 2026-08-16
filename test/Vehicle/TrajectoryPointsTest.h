#pragma once

#include "UnitTest.h"

class TrajectoryPointsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _firstPointAndDecimation();
    void _verticalMotion();
    void _nanAltitude();
    void _flightDistance3D();
    void _clearResets();
};
