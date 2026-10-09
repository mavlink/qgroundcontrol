#pragma once

#include "UnitTest.h"

/// The receiver settings and status panels of the GNSS Receiver settings page, loaded outside the main window.
class GPSReceiverSettingsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _surveySaveWorkflow();
    void _surveyCompletePrompt();
    void _reconnectingOffersDisconnect();
    void _tcpConnectionFields();
    void _roleSelectsFields();
    void _automaticManufacturer();
    void _automaticOffersQuectelConsent();
    void _compactCorrectionsToggle();
    void _consentIsOneUse();
    void _warningWidth_data();
    void _warningWidth();
    void _serialSelectionTracksFacts();
    void _horizontalAccuracyLabel();
};
