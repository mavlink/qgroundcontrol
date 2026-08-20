#pragma once

#include "UnitTest.h"

class GeoMapCameraTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _defaults();
    void _positionedOnExplicitCenter();
    void _clamps();
    void _fieldOfView();
    void _reset();
    void _cameraPositionConvention();
    void _screenToGroundTopDown();
    void _screenToGroundHorizonMiss();
    void _screenToGroundNoViewport();
    void _groundPointCapped();
    void _worldToScreen();
    void _sceneUnitsPerPixel();
    void _distanceForZoomLevel();
    void _centerForCoordinateAtScreenPoint();
    void _centerElevation();
    void _panAnchorInvariant_data();
    void _panAnchorInvariant();
    void _zoomAnchorInvariant_data();
    void _zoomAnchorInvariant();
    void _rotateAnchorInvariant();
    void _tiltAnchorInvariant();
    void _orbitDragRatios();
    void _orbitSurfaceAnchorInvariant_data();
    void _orbitSurfaceAnchorInvariant();
    void _lookKeepsCameraFixed();
    void _lookHeadingOnlyIn2D();
    void _modeSwitch();
    void _tiltLockedIn2D();
    void _scenePose();
};
