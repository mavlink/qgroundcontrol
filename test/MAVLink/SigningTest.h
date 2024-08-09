/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "UnitTest.h"

class SigningTest : public UnitTest
{
    Q_OBJECT

public:
    SigningTest() = default;

private slots:
    void _testInitSigning();
    void _testCheckSigningLinkId();
    void _testCreateSetupSigning();
};
