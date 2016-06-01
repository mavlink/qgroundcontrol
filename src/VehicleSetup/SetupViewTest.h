/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#ifndef SetupViewTest_H
#define SetupViewTest_H

#include "UnitTest.h"
#include "MainWindow.h"

/// Click through test for Setup View buttons
class SetupViewTest : public UnitTest
{
    Q_OBJECT
    
private slots:
    void _clickThrough_test(void);
};

#endif
