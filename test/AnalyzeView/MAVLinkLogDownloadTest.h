#pragma once

#include "BaseClasses/VehicleTest.h"

class MAVLinkLogDownloadTest : public VehicleTest
{
    Q_OBJECT

private slots:
    void _downloadTest();
};
