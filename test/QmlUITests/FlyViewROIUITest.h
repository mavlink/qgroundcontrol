#pragma once

#include "QmlUITestBase.h"

/// End-to-end UI test for Click to ROI. Connects a MockLink vehicle, puts it in
/// the air, clicks the map, chooses "ROI at location" from the drop panel,
/// verifies the altitude slider appears, sets a slider value, confirms the action,
/// and checks the MAV_CMD_DO_SET_ROI_LOCATION command received by MockLink.
class FlyViewROIUITestBase : public QmlUITestBase
{
    Q_OBJECT

protected:
    void _runROIFromMapClick(bool apmFirmware, bool geoMapEngine = false);
};

/// PX4: altitude converted to AMSL (home + relative) and sent in MAV_FRAME_GLOBAL
class FlyViewROIUITest : public FlyViewROIUITestBase
{
    Q_OBJECT

private slots:
    void _testROIFromMapClick();
};

/// ArduPilot: relative altitude sent unconverted in MAV_FRAME_GLOBAL_RELATIVE_ALT
class FlyViewROIAPMUITest : public FlyViewROIUITestBase
{
    Q_OBJECT

private slots:
    void init() override;
    void _testROIFromMapClick();
};

/// Same click-to-ROI flow with the experimental GeoMap fly view engine enabled:
/// the click resolves through the GeoMap camera inverse projection instead of
/// QtLocation (PX4 firmware, 2D camera mode).
class FlyViewGeoROIUITest : public FlyViewROIUITestBase
{
    Q_OBJECT

private slots:
    void _testROIFromMapClick();
};
