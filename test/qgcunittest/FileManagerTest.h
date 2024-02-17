/****************************************************************************
 *
 *   (c) 2009-2018 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#pragma once

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
    static const int _ackTimerTimeoutMsecs = FileManager::ackTimerMaxRetries * FileManager::ackTimerTimeoutMsecs * 2;
    
    QStringList _fileListReceived;
};

