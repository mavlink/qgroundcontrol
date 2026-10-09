#pragma once

#include "Protocols/Support/GPSProtocolTestBase.h"

/// The Femtomes family: command acknowledgements, base configuration and mixed framing.
class FemtoProtocolTest : public GPSProtocolTestBase
{
    Q_OBJECT

private slots:
    void _scenario_data();
    void _scenario();
    void _receiverMode_data();
    void _receiverMode();
};
