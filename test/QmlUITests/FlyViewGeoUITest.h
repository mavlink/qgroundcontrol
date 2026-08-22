#pragma once

#include "QmlUITestBase.h"

/// UI test for the experimental GeoMap Fly View engine (preview feature).
///
/// Verifies that enabling the reboot-required engine setting before startup
/// loads the GeoMap adapter as the Fly View map, that toggling the setting at
/// runtime does NOT swap the engine, and that camera gestures (pan, orbit,
/// wheel zoom, pinch zoom, two-finger twist) drive the camera correctly.
class FlyViewGeoUITest : public QmlUITestBase
{
    Q_OBJECT

private slots:
    void _testEngineEnabledAtStartup();
    void _testFlyViewEngineSwap();
    void _testCameraGestures();
    void _testModeToggleAndCompass();
    void _testDebugHillsToggle();
};
