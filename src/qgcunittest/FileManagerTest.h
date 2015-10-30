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

#ifndef FileManagerTEST_H
#define FileManagerTEST_H

#include <QObject>
#include <QtTest/QtTest>

#include "UnitTest.h"
#include "FileManager.h"
#include "MockLink.h"
#include "MultiSignalSpy.h"

/// @file
///     @brief FileManager unit test
///
///     @author Don Gagne <don@thegagnes.com>

class FileManagerTest : public UnitTest
{
    Q_OBJECT
    
public:
    FileManagerTest(void);
    
private slots:
    // Test case initialization
    void init(void);
    void cleanup(void);
    
    // Test cases
    void _ackTest(void);
    void _noAckTest(void);
    void _listTest(void);
	
    // Connected to FileManager listEntry signal
    void listEntry(const QString& entry);
    
private:
    void _validateFileContents(const QString& filePath, uint8_t length);

    enum {
        listEntrySignalIndex = 0,
        commandCompleteSignalIndex,
        commandErrorSignalIndex,
        maxSignalIndex
    };
    
    enum {
        listEntrySignalMask =       1 << listEntrySignalIndex,
        commandCompleteSignalMask = 1 << commandCompleteSignalIndex,
        commandErrorSignalMask =    1 << commandErrorSignalIndex,
    };

    static const uint8_t    _systemIdQGC = 255;
    static const uint8_t    _systemIdServer = 128;

    MockLinkFileServer* _fileServer;
    FileManager*        _fileManager;

    MultiSignalSpy*     _multiSpy;
    static const size_t _cSignals = maxSignalIndex;
    const char*         _rgSignals[_cSignals];
    
    /// @brief This is the amount of time to wait to allow the FileManager enough time to timeout waiting for an Ack.
    /// As such it must be larger than the Ack Timeout used by the FileManager.
    static const int _ackTimerTimeoutMsecs = FileManager::ackTimerTimeoutMsecs * 2;
    
    QStringList _fileListReceived;
};

#endif
