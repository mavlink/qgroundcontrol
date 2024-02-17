/****************************************************************************
 *
 *   (c) 2009-2018 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @brief Simple MainWindow unit test
///
///     @author Don Gagne <don@thegagnes.com>

#pragma once

#include "UnitTest.h"
#include "MainWindow.h"

class MainWindowTest : public UnitTest
{
    Q_OBJECT
    
private slots:
    void _connectWindowClosePX4_test(void);
    void _connectWindowCloseGeneric_test(void);

private:
    void _connectWindowClose_test(MAV_AUTOPILOT autopilot);
};

