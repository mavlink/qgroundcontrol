#pragma once

#include "UnitTest.h"

class GeoMapItemTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _positionTracksProjection();
    void _anchorPointOffset();
    void _notProjectedParksOffScreen();
    void _repositionsOnCameraPose();
    void _terrainScaleScalesAltitude();
    void _clampToGroundTracksTerrain();
    void _scenePosition();
    void _scenePositionShiftsOnReanchor();
    void _delegate3DLifecycle();
    void _delegate3DWaitsForContainer();
    void _node3DVisibilityTracksItem();
};
