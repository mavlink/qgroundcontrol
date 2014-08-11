/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

#ifndef QGCUASFILEMANAGERTEST_H
#define QGCUASFILEMANAGERTEST_H

#include <QObject>
#include <QtTest/QtTest>

#include "AutoTest.h"
#include "MockUAS.h"
#include "MockMavlinkFileServer.h"
#include "QGCUASFileManager.h"
#include "MultiSignalSpy.h"

/// @file
///     @brief QGCUASFileManager unit test
///
///     @author Don Gagne <don@thegagnes.com>

class QGCUASFileManagerUnitTest : public QObject
{
    Q_OBJECT
    
public:
    QGCUASFileManagerUnitTest(void);
    
private slots:
    // Test case initialization
    void initTestCase(void);
    void init(void);
    void cleanup(void);
    
    // Test cases
    void _ackTest(void);
    void _noAckTest(void);
    void _resetTest(void);
    void _listTest(void);
    void _downloadTest(void);
    
    // Connected to QGCUASFileManager statusMessage signal
    void statusMessage(const QString&);
    
private:
    void _validateFileContents(const QString& filePath, uint8_t length);

    enum {
        statusMessageSignalIndex = 0,
        errorMessageSignalIndex,
        resetStatusMessagesSignalIndex,
        listCompleteSignalIndex,
        openFileLengthSignalIndex,
        maxSignalIndex
    };
    
    enum {
        statusMessageSignalMask =           1 << statusMessageSignalIndex,
        errorMessageSignalMask =            1 << errorMessageSignalIndex,
        resetStatusMessagesSignalMask =     1 << resetStatusMessagesSignalIndex,
        listCompleteSignalMask =            1 << listCompleteSignalIndex,
        openFileLengthSignalMask =          1 << openFileLengthSignalIndex,
    };
    
    MockUAS                 _mockUAS;
    MockMavlinkFileServer   _mockFileServer;
    
    QGCUASFileManager*  _fileManager;

    MultiSignalSpy*     _multiSpy;
    static const size_t _cSignals = maxSignalIndex;
    const char*         _rgSignals[_cSignals];
    
    /// @brief This is the amount of time to wait to allow the FileManager enough time to timeout waiting for an Ack.
    /// As such it must be larger than the Ack Timeout used by the FileManager.
    static const int _ackTimerTimeoutMsecs = QGCUASFileManager::ackTimerTimeoutMsecs * 2;
    
    QStringList _fileListReceived;
};

DECLARE_TEST(QGCUASFileManagerUnitTest)

#endif
