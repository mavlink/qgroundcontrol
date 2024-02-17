/****************************************************************************
 *
 *   (c) 2009-2018 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @brief Unit test for QGCMessageBox catching mechanism.
///
///     @author Don Gagne <don@thegagnes.com>

#pragma once

#include "UnitTest.h"

class MessageBoxTest : public UnitTest
{
    Q_OBJECT
    
public:
    MessageBoxTest(void);
    
private slots:
    void _messageBoxExpected_test(void);
    void _messageBoxUnexpected_test(void);
    void _previousMessageBox_test(void);
    void _noMessageBox_test(void);
    void _badResponseButton_test(void);
};

