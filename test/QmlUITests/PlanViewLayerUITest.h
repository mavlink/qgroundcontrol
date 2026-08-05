#pragma once

#include "QmlUITestBase.h"

/// UI test for Plan view editing layer switching (offline, no vehicle).
///
/// Verifies the accordion behavior of the Mission/GeoFence/Rally tree groups,
/// the layer switcher tool, create-from-template gating, and that the map
/// visuals' interactive state follows the active editing layer.
class PlanViewLayerUITest : public QmlUITestBase
{
    Q_OBJECT

public:
    PlanViewLayerUITest() = default;

private slots:
    void _testEditingLayerSwitching();

private:
    /// Complete expected state of the layer-related UI
    struct LayerUIState {
        int  editingLayer;          ///< expected PlanView._editingLayer (1=Mission 2=Fence 3=Rally)
        bool switcherVisible;       ///< layer switcher tool visible (hidden in create-from-template mode)
        bool missionEditorVisible;  ///< a mission item editor is visible (mission group expanded, plan non-empty)
        bool fenceEditorVisible;    ///< the fence editor is visible (fence group expanded)
        bool rallyHeaderVisible;    ///< the rally editor header is visible (rally group expanded)
        bool fenceInteractive;      ///< GeoFenceMapVisuals.interactive
        bool rallyInteractive;      ///< RallyPointMapVisuals.interactive
    };

    /// Verify the full layer UI state against expectations
    void _verifyLayerState(const LayerUIState &state, const QString &context);

    /// Current PlanView._editingLayer value, or -1 if unavailable
    int _editingLayer();

    /// Click the Plan view map at a fractional position within its viewport
    void _clickMap(qreal fractionX, qreal fractionY);

    /// Scroll a tree row into view and click it
    bool _clickTreeRow(const QString &objectName);
};
