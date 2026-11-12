#pragma once

#include "UnitTest.h"

/// The receiver settings and status panels of the GNSS Receiver settings page, loaded outside the main window.
class GPSReceiverSettingsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _surveySaveWorkflow();
    void _surveyCompletePrompt();
    void _surveyProgress_data();
    void _surveyProgress();
    void _basePositionShownOnceValid_data();
    void _basePositionShownOnceValid();
    void _surveySaveNeedsApplicationReceiver();
    void _surveySavedAsSite();
    void _baseSitePicker();
    void _statusListsConstellations();
    void _connectionRowFollowsReceiver();
    void _tcpConnectionFields_data();
    void _tcpConnectionFields();
    void _roleSelectsFields();
    void _automaticManufacturer();
    void _automaticOffersQuectelConsent();
    void _compactCorrectionsToggle();
    void _recordingSwitch();
    void _consentIsOneUse();
    void _warningWidth_data();
    void _warningWidth();
    void _serialSelectionTracksFacts();
    void _serialSelectionSurvivesChoiceChanges();
    void _serialDeviceMarkOnNarrowPanel_data();
    void _serialDeviceMarkOnNarrowPanel();
    void _horizontalAccuracyLabel();
    void _settingsPageListsGroups_data();
    void _settingsPageListsGroups();
};
