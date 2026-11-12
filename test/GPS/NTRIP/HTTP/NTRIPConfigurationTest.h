#pragma once

#include "UnitTest.h"

class NTRIPConfigurationTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _validity_data();
    void _validity();
    void _endpointJoinsHostAndPort_data();
    void _endpointJoinsHostAndPort();
};
