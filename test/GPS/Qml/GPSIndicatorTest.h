#pragma once

#include "UnitTest.h"

/// The toolbar GPS indicator and its drawer page, loaded outside the main window: vehicle GPS and resilience, the GNSS
/// receiver without vehicle GPS, and the receiver settings and NTRIP connection the drawer offers.
class GPSIndicatorTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _pagePanelUsesApplicationReceiver();
    void _pageFitsWidth_data();
    void _pageFitsWidth();
    void _compactExpandedPageHasSettingsSize();
    void _gcsPositionStaysInsideItsBox();
    void _ntripStaysInsideItsBox();
    void _receiverStaysInsideItsBox();
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
