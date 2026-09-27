#pragma once

#include "UnitTest.h"

class AudioOutputTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testSpokenReplacements();
    void _abbreviationsReplacedAtTokenBoundaries_data();
    void _abbreviationsReplacedAtTokenBoundaries();
};
