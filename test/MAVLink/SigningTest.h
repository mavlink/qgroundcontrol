#pragma once

#include "UnitTest.h"

class SigningTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testInitSigning();
    void _testCheckSigningLinkId();
    void _testCreateSetupSigning();
};
