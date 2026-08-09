#pragma once

#include "UnitTest.h"

class GeoViewCameraTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _defaults();
    void _clamps();
    void _fieldOfView();
    void _reset();
    void _cameraPositionConvention();
    void _screenToGroundTopDown();
    void _screenToGroundHorizonMiss();
    void _screenToGroundNoViewport();
    void _groundPointCapped();
    void _sceneUnitsPerPixel();
    void _panAnchorInvariant_data();
    void _panAnchorInvariant();
    void _zoomAnchorInvariant();
    void _rotateAnchorInvariant();
    void _tiltAnchorInvariant();
    void _orbitDragRatios();
    void _modeSwitch();
    void _tiltLockedIn2D();
    void _scenePose();
};
