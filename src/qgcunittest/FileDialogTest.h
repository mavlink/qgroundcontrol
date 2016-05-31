/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @brief Unit test for QGCFileDialog catching mechanism.
///
///     @author Don Gagne <don@thegagnes.com>

#ifndef FILEDIALOGTEST_H
#define FILEDIALOGTEST_H

#include "UnitTest.h"

class FileDialogTest : public UnitTest
{
    Q_OBJECT
    
public:
    FileDialogTest(void);
    
private slots:
    void _fileDialogExpected_test(void);
    void _fileDialogUnexpected_test(void);
    void _previousFileDialog_test(void);
    void _noFileDialog_test(void);
    void _fileDialogExpectedIncorrect_test(void);
};

#endif
