#pragma once

#include "UnitTest.h"

class AudioOutputTest : public UnitTest
{
    Q_OBJECT

private slots:
    UT_PARAMETERIZED_TEST(_testSpokenReplacements);
    UT_PARAMETERIZED_TEST(_abbreviationsReplacedAtTokenBoundaries);
};
