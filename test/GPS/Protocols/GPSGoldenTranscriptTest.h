#pragma once

#include "UnitTest.h"

class GPSGoldenTranscriptTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _scenario_data();
    void _scenario();
    void _decode_data();
    void _decode();
    void _decodeOnlyArming_data();
    void _decodeOnlyArming();
    void _inventory();
};
