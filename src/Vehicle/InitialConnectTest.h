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
#include "MockLink.h"
#include "Vehicle.h"

class InitialConnectTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _performTestCases(void);
    void _boardVendorProductId(void);
};
