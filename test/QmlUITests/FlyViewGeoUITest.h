#pragma once

#include "QmlUITestBase.h"

/// UI test for the experimental FlyViewGeo top-level view (preview feature).
///
/// Verifies the GeoView entry is hidden from the view-select dropdown when the
/// preview setting is disabled, that enabling it makes the entry visible and
/// switches to the FlyViewGeo main panel, and that camera gestures (pan, orbit,
/// wheel zoom, pinch zoom, two-finger twist) drive the camera correctly.
class FlyViewGeoUITest : public QmlUITestBase
{
    Q_OBJECT

private slots:
    void _testHiddenWhenDisabled();
    void _testViewSwitchWhenEnabled();
    void _testCameraGestures();
    void _testModeToggleAndCompass();
    void _testDebugHillsToggle();
};
