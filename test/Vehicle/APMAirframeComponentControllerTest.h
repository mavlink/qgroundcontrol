#pragma once

#include "BaseClasses/VehicleTest.h"

class APMAirframeComponentControllerTest : public VehicleTestAPM
{
    Q_OBJECT

public:
    APMAirframeComponentControllerTest();

private slots:
    void _downloadCompleteSlotsRestoreCursor();
};
