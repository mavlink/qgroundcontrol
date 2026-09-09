#pragma once

#include "UnitTest.h"

class GPSReceiverCapabilitiesTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _settingDescriptors();
    void _familyResolution_data();
    void _familyResolution();
    void _configurationSupport_data();
    void _configurationSupport();
    void _detectedCapabilities();
};
