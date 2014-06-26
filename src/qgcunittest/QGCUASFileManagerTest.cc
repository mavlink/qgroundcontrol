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

#include "QGCUASFileManagerTest.h"

/// @file
///     @brief QGCUASFileManager unit test
///
///     @author Don Gagne <don@thegagnes.com>

QGCUASFileManagerUnitTest::QGCUASFileManagerUnitTest(void) :
    _fileManager(NULL),
    _multiSpy(NULL)
{
    
}

// Called once before all test cases are run
void QGCUASFileManagerUnitTest::initTestCase(void)
{
    _mockUAS.setMockMavlinkPlugin(&_mockFileServer);
}

// Called before every test case
void QGCUASFileManagerUnitTest::init(void)
{
    Q_ASSERT(_multiSpy == NULL);
    
    _fileManager = new QGCUASFileManager(NULL, &_mockUAS);
    Q_CHECK_PTR(_fileManager);
    
    bool connected = connect(&_mockFileServer, SIGNAL(messageReceived(LinkInterface*, mavlink_message_t)), _fileManager, SLOT(receiveMessage(LinkInterface*, mavlink_message_t)));
    Q_ASSERT(connected);

    connected = connect(_fileManager, SIGNAL(statusMessage(const QString&)), this, SLOT(statusMessage(const QString&)));
    Q_ASSERT(connected);

    _rgSignals[statusMessageSignalIndex] = SIGNAL(statusMessage(const QString&));
    _rgSignals[errorMessageSignalIndex] = SIGNAL(errorMessage(const QString&));
    _rgSignals[resetStatusMessagesSignalIndex] = SIGNAL(resetStatusMessages(void));
    
    _multiSpy = new MultiSignalSpy();
    Q_CHECK_PTR(_multiSpy);
    QCOMPARE(_multiSpy->init(_fileManager, _rgSignals, _cSignals), true);
}

// Called after every test case
void QGCUASFileManagerUnitTest::cleanup(void)
{
    Q_ASSERT(_multiSpy);
    Q_ASSERT(_fileManager);
    
    delete _fileManager;
    delete _multiSpy;
    
    _fileManager = NULL;
    _multiSpy = NULL;
}

/// @brief Connected to QGCUASFileManager statusMessage signal in order to catch list command output
void QGCUASFileManagerUnitTest::statusMessage(const QString& msg)
{
    // Keep a list of all names received so we can test it for correctness
    _fileListReceived += msg;
}


void QGCUASFileManagerUnitTest::_ackTest(void)
{
    Q_ASSERT(_fileManager);
    Q_ASSERT(_multiSpy);
    Q_ASSERT(_multiSpy->checkNoSignals() == true);
    
    // If the file manager doesn't receive an ack it will timeout and emit an error. So make sure
    // we don't get any error signals.
    QVERIFY(_fileManager->_sendCmdTestAck());
    QVERIFY(_multiSpy->checkNoSignals());
}

void QGCUASFileManagerUnitTest::_noAckTest(void)
{
    Q_ASSERT(_fileManager);
    Q_ASSERT(_multiSpy);
    Q_ASSERT(_multiSpy->checkNoSignals() == true);
    
    // This should not get the ack back and timeout.
    QVERIFY(_fileManager->_sendCmdTestNoAck());
    QTest::qWait(2000); // Let the file manager timeout, magic number 2 secs must be larger than file manager ack timeout
    QCOMPARE(_multiSpy->checkOnlySignalByMask(errorMessageSignalMask), true);
}

void QGCUASFileManagerUnitTest::_resetTest(void)
{
    Q_ASSERT(_fileManager);
    Q_ASSERT(_multiSpy);
    Q_ASSERT(_multiSpy->checkNoSignals() == true);
    
    // Send a reset command
    //  We should not get any signals back from this
    QVERIFY(_fileManager->_sendCmdReset());
    QVERIFY(_multiSpy->checkNoSignals());
}

void QGCUASFileManagerUnitTest::_listTest(void)
{
    Q_ASSERT(_fileManager);
    Q_ASSERT(_multiSpy);
    Q_ASSERT(_multiSpy->checkNoSignals() == true);
    
    // Send a bogus path
    //  We should get a single resetStatusMessages signal
    //  We should get a single errorMessage signal
    _fileManager->listRecursively("/bogus");
    QCOMPARE(_multiSpy->checkOnlySignalByMask(errorMessageSignalMask | resetStatusMessagesSignalMask), true);
    _multiSpy->clearAllSignals();

    // Send a list command at the root of the directory tree
    //  We should get back a single resetStatusMessages signal
    //  We should not get back an errorMessage signal
    //  We should get back one or more statusMessage signals
    //  The returned list should match out inputs
    
    QStringList fileList;
    fileList << "Ddir" << "Ffoo" << "Fbar";
    _mockFileServer.setFileList(fileList);
    
    QStringList fileListExpected;
    fileListExpected << "dir/" << "foo" << "bar";
    
    _fileListReceived.clear();
    
    _fileManager->listRecursively("/");
    QCOMPARE(_multiSpy->checkSignalByMask(resetStatusMessagesSignalMask), true);  // We should be told to reset status messages
    QCOMPARE(_multiSpy->checkNoSignalByMask(errorMessageSignalMask), true);  // We should not get an error signals
    QVERIFY(_fileListReceived == fileListExpected);
}
