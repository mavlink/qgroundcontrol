#pragma once

#include "UnitTest.h"

class GPSReceiverSettingsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _surveySaveWorkflow_data();
    void _surveySaveWorkflow();
    void _unavailablePositionCannotBeSaved_data();
    void _unavailablePositionCannotBeSaved();
    void _reconnectingOffersDisconnect();
    void _tcpConnectionFields();
    void _roleSelectsFields();
    void _compactCorrectionsToggle();
    void _consentIsOneUse();
    void _indicatorConsentTracksSettings();
    void _warningWidth_data();
    void _warningWidth();
    void _pageWidth_data();
    void _pageWidth();
    void _disconnectedPage_data();
    void _disconnectedPage();
    void _serialSelectionTracksFacts();
    void _resilienceUnknownStates_data();
    void _resilienceUnknownStates();
    void _indicatorShowsReceiverWithoutVehicleGps_data();
    void _indicatorShowsReceiverWithoutVehicleGps();
    void _resiliencePageGroups();
    void _horizontalAccuracyLabel();
    void _vehicleAccuracyFacts();
};
