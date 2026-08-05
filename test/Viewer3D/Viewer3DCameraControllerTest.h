/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "UnitTest.h"

class Viewer3DCameraControllerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testDefaults();
    void _testReset();
    void _testLookAt();
    void _testLookAtClamps();
    void _testTiltSetterClamp();
    void _testDistanceSetterClamp();
    void _testCameraPositionTopDown();
    void _testCameraPositionTilted();
    void _testCameraPositionHeading();
    void _testScreenToGroundNoViewport();
    void _testScreenToGroundCenter();
    void _testScreenToGroundOffCenter();
    void _testScreenToGroundMiss();
    void _testPanKeepsAnchorUnderCursor();
    void _testPanPreservesPose();
    void _testOrbitYawAngle();
    void _testOrbitTiltAngle();
    void _testOrbitTiltClamped();
    void _testOrbitOffCenterAnchor();
    void _testOrbitPreservesDistance();
    void _testRotateByChangesHeading();
    void _testRotateByKeepsAnchorUnderTouch();
    void _testTiltByChangesTilt();
    void _testTiltByClamped();
    void _testTiltByKeepsAnchorUnderTouch();
    void _testZoomChangesDistance();
    void _testZoomRoundTrip();
    void _testZoomByFactor();
    void _testZoomTowardCursorKeepsAnchor();
    void _testZoomClamp();
    void _testZoomAboveHorizonFallback();
    void _testSceneUnitsPerPixel();
    void _testGesturesWithoutViewportNoop();
};
