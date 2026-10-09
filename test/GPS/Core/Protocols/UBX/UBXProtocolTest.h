#pragma once

#include "Protocols/Support/GPSProtocolTestBase.h"

/// The u-blox family: wire codecs, discovery, base configuration, survey stop, diagnostics and recorded fixtures.
class UBXProtocolTest : public GPSProtocolTestBase
{
    Q_OBJECT

private slots:
    void _scenario_data();
    void _scenario();
    void _surveyStop_data();
    void _surveyStop();
    void _replyFaults_data();
    void _replyFaults();
};
