#pragma once

#include "UnitTest.h"

class PX4FlightModeConfigTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _selectionMethod_data();
    void _selectionMethod();
    void _buttonChannels();
    void _externalModeIndex_data();
    void _externalModeIndex();
    void _modeChoices_data();
    void _modeChoices();
    void _modeName();
};
