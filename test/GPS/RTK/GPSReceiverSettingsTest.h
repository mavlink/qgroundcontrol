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
    void _consentIsOneUse();
    void _warningWidth_data();
    void _warningWidth();
    void _pageWidth_data();
    void _pageWidth();
    void _disconnectedPage_data();
    void _disconnectedPage();
};
