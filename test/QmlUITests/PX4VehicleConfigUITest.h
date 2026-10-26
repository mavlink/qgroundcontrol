#pragma once

#include "VehicleConfigUITestBase.h"

class MockLink;

/// UI smoke test that boots the full QML UI with a PX4 MockLink vehicle
/// connected and navigates through all vehicle configuration pages.
class PX4VehicleConfigUITest : public VehicleConfigUITestBase
{
    Q_OBJECT

public:
    PX4VehicleConfigUITest() = default;

private slots:
    void _testNavigateVehicleConfig();
    void _testDisconnectWithPIDTuningOpen();
    void _testSidebarDividers();
    void _testSearchMatchesComponentKeywords();

protected:
    // The Advanced tab of the Sensors page. Its title uses SetupPage's "Advanced" translation, so the text follows the
    // language.
    QString languageAnchorObjectName() const override { return QStringLiteral("sensors_tab_advanced"); }

private:
    /// Click each axis button in \a axisNames in order, waiting for a polish
    /// pass between each click.  Used by _testDisconnectWithPIDTuningOpen().
    void _cycleAxisButtons(const QStringList &axisNames);
};
