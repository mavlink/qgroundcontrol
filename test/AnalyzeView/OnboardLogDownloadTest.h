#pragma once

#include "BaseClasses/VehicleTest.h"

class OnboardLogDownloadTest : public VehicleTest
{
    Q_OBJECT

private slots:
    void _downloadTest();
};
