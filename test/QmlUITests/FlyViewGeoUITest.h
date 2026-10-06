#pragma once

#include "QmlUITestBase.h"

/// UI test for the experimental GeoMap Fly View engine (preview feature).
///
/// Verifies that enabling the reboot-required engine setting before startup
/// loads the GeoMap adapter as the Fly View map, that toggling the setting at
/// runtime does NOT swap the engine, and that camera gestures (pan, orbit,
/// wheel zoom, pinch zoom, two-finger twist) drive the camera correctly. Also
/// covers the 2D/3D mode toggle and compass, orbit/loiter circle editing, and
/// restoring the map position and zoom at startup and sharing them with Plan view,
/// including across window resizes and PiP swaps, zooming to a mission
/// downloaded from the vehicle, and picking among overlapping waypoint markers.
class FlyViewGeoUITest : public QmlUITestBase
{
    Q_OBJECT

private slots:
    void _testEngineEnabledAtStartup();
    void _testFlyViewEngineSwap();
    void _testCameraGestures();
    void _testModeToggleAndCompass();
    void _testDebugHillsToggle();
    void _testCircleEditHandles_data();
    void _testCircleEditHandles();
    void _testLoiterRadiusEdit();
    void _testMapViewSharedWithPlanView();
    void _testResizeKeepsSharedZoom();
    void _testPipExitAppliesSharedZoom();
    void _testZoomToMissionFromVehicle_data();
    void _testZoomToMissionFromVehicle();
    void _testOverlappingMarkersPicker_data();
    void _testOverlappingMarkersPicker();
};
