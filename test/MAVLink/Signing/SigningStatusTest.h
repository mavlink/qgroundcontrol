#pragma once

#include "UnitTest.h"

class SigningStatusTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testDefaults();
    void _testPendingDerivation();
    void _testEquality();
    void _testEnumValues();
    void _testRegisteredAsMetaType();
};
