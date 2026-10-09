#pragma once

#include "Protocols/Support/GPSProtocolTestBase.h"

/// The Quectel family: identity and role safety, survey lifecycle, managed persistence and the NMEA codec.
class QuectelProtocolTest : public GPSProtocolTestBase
{
    Q_OBJECT

private slots:
    void _scenario_data();
    void _scenario();
    void _commandFaults_data();
    void _commandFaults();
    void _managedFaults_data();
    void _managedFaults();
    void _codec();
};
