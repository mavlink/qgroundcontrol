#pragma once

#include "UnitTest.h"

/// The toolbar GPS indicator and its drop-down page, loaded outside the main window: vehicle GPS and resilience, the
/// GNSS receiver without vehicle GPS, and the status groups, the actions and the links to the settings pages that the
/// drop-down holds. ToolbarIndicatorUITest follows the links in the main window.
class GPSIndicatorTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _pageFitsWidth_data();
    void _pageFitsWidth();
    void _holdsStatusAndActions();
    void _gcsPositionStaysInsideItsBox();
    void _ntripStaysInsideItsBox();
    void _receiverStaysInsideItsBox();
    void _vehicleStaysInsideItsBox_data();
    void _vehicleStaysInsideItsBox();
    void _vehicleRtkValuesShownOnce();
    void _correctionsStayInsideTheirBox_data();
    void _correctionsStayInsideTheirBox();
    void _statusColumnKeepsItsWidth();
    void _receiverBesideVehicleGps();
    void _pageOffersNtripConnect();
    void _pageVehicleMeasurements();
    void _pageResilienceGroups();
    void _resilienceIcon_data();
    void _resilienceIcon();
    void _showsReceiverWithoutVehicleGPS_data();
    void _showsReceiverWithoutVehicleGPS();
    void _labelFollowsVehicleFix_data();
    void _labelFollowsVehicleFix();
    void _receiverAntennaWarning();
};
