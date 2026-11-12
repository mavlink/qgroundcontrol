#pragma once

#include "UnitTest.h"

class NTRIPConfigurationTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _validity_data();
    void _validity();
    void _endpointKeepsPlaceMarkers_data();
    void _endpointKeepsPlaceMarkers();
};
