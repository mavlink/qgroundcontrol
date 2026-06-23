#pragma once

#include "VehicleConfigUITestBase.h"

class QString;
class Vehicle;

/// UI test that boots the full QML UI with a PX4 MockLink vehicle connected
/// whose parameters (including SYS_AUTOSTART) are reset to the firmware
/// defaults, then exercises the Airframe setup page.
class PX4AirframeSetupUITest : public VehicleConfigUITestBase
{
    Q_OBJECT

public:
    PX4AirframeSetupUITest() = default;

private slots:
    void _testNavigateToAirframe();
    void _testAirframePrereqPages();
    void _testApplyAirframe();

private:
    /// Navigate from the Fly view to the Airframe page of the Configure view
    /// and verify the airframe-selection page is shown.
    void _navigateToAirframePanel();

    /// Click the given component button in the config sidebar and verify whether
    /// the Airframe prerequisite-setup message panel is shown. Expandable
    /// components must expand even when the prerequisite is not met. When
    /// checkedSectionObjectName is non-empty, that first child section must be
    /// selected after the click.
    void _verifyAirframePrereq(const QString &compObjectName, bool expectPrereqShown, const QString &checkedSectionObjectName);
};
