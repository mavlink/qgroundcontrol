#pragma once

#include "UnitTest.h"

class GeoMapProjectedPathTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _projectsAbsolute();
    void _clampToGroundTracksTerrain();
    void _failedProjections();
    void _reprojectsOnCameraPose();
    void _cameraReplacement();
    void _sceneAndModelDestruction();
    void _circleCoordinates();
};
