#pragma once

#include "VehicleConfigUITestBase.h"

class QString;
class Vehicle;

/// UI test that boots the full QML UI with a PX4 MockLink vehicle connected,
/// then exercises the Airframe setup page. Most tests reset the vehicle's
/// parameters (including SYS_AUTOSTART) to the firmware defaults first.
class PX4AirframeSetupUITest : public VehicleConfigUITestBase
{
    Q_OBJECT

public:
    PX4AirframeSetupUITest() = default;

private slots:
    void _testNavigateToAirframe();
    void _testCurrentAirframeCategorySelected();
    void _testAirframePrereqPages();
    void _testApplyAirframe();
    void _testPendingAirframeDiscarded();

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
