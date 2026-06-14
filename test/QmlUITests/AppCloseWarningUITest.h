#pragma once

#include "QmlUITestBase.h"

/// UI tests for the application close warning dialogs.
///
/// On close MainWindow runs a sequence of checks (unsaved mission, pending
/// parameter writes, active vehicle connections), each of which raises its own
/// warning dialog in that fixed order. Accepting a warning advances to the next
/// check; rejecting one cancels the close. The matrix test drives every
/// reachable permutation of the three conditions and every accept/reject
/// decision through the real toolbar Q-logo dropdown Close button.
///
/// A separate test verifies that a freshly downloaded (unedited) mission does
/// NOT raise the unsaved-mission warning, because a downloaded plan reflects
/// exactly what is on the vehicle and must not be considered dirty.
class AppCloseWarningUITest : public QmlUITestBase
{
    Q_OBJECT

public:
    AppCloseWarningUITest() = default;

private slots:
    void _testCloseWarningMatrix_data();
    void _testCloseWarningMatrix();
    void _testNoUnsavedMissionWarningForDownloadedMission();

private:
    /// Force the Plan view's (offline) mission into a dirty-for-save state so the
    /// unsaved-mission close warning fires, without editing the mission through
    /// the UI. Returns false (after recording a test failure) if the plan
    /// controller cannot be reached.
    bool _forcePlanViewMissionDirty();
};
