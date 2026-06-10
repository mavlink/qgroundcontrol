#pragma once

#include "QmlUITestBase.h"

class QQuickItem;

/// UI test for Plan view state transitions (offline, no vehicle).
///
/// Walks the Plan view through its full editing lifecycle verifying at each
/// step: template-creation availability, toolstrip button enablement, and
/// Save/Upload button enabled/highlight state.
class PlanViewUITest : public QmlUITestBase
{
    Q_OBJECT

public:
    PlanViewUITest() = default;

private slots:
    void _testPlanViewStates();

private:
    /// Complete expected state of all Plan view UI under test
    struct PlanUIState {
        bool templatesVisible;
        bool templatesEnabled;  ///< only checked when templatesVisible
        bool takeoffEnabled;
        bool waypointEnabled;
        bool waypointChecked;   ///< add-waypoint-on-click mode active
        bool patternEnabled;
        bool roiEnabled;        ///< only checked if ROI button is visible (vehicle support)
        bool roiChecked;        ///< add-ROI-on-click mode active
        bool roiCancelText;     ///< ROI button reads "Cancel ROI" (an ROI is active in the plan)
        bool landEnabled;
        QString landText;       ///< expected Land button label ("Return" for multirotor)
        bool saveEnabled;
        bool savePrimary;
        int itemCount;          ///< expected visualItems.count (mission settings item counts as 1)
    };

    /// Verify every Plan view UI element against the full expected state.
    /// Also checks the invariants that hold while offline: Upload disabled
    /// and not highlighted, Open/Clear always enabled.
    void _verifyFullState(const PlanUIState &state, const QString &context);

    /// Click the Plan view map at a fractional position within its viewport.
    /// (0.5, 0.5) is the map center.
    void _clickMap(qreal fractionX, qreal fractionY);

    /// Current MissionController visualItems count, or -1 if unavailable.
    int _missionItemCount();
};
