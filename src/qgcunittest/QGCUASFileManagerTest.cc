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
///     @brief QGCUASFileManager unit test. Note: All code here assumes all work between
///             the unit test, mack mavlink file server and file manager is happening on
///             the same thread.
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
    _fileManager->listDirectory("/bogus");
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
    
    _fileManager->listDirectory("/");
    QCOMPARE(_multiSpy->checkSignalByMask(resetStatusMessagesSignalMask), true);  // We should be told to reset status messages
    QCOMPARE(_multiSpy->checkNoSignalByMask(errorMessageSignalMask), true);  // We should not get an error signals
    QVERIFY(_fileListReceived == fileListExpected);
}

void QGCUASFileManagerUnitTest::_validateFileContents(const QString& filePath, uint8_t length)
{
    QFile file(filePath);

    // Make sure file size is correct
    QCOMPARE(file.size(), (qint64)length);

    // Read data
    QVERIFY(file.open(QIODevice::ReadOnly));
    QByteArray bytes = file.readAll();
    file.close();
    
    // Validate length byte
    QCOMPARE((uint8_t)bytes[0], length);
    
    // Validate file contents:
    //      Repeating 0x00, 0x01 .. 0xFF until file is full
    for (uint8_t i=1; i<bytes.length(); i++) {
        QCOMPARE((uint8_t)bytes[i], (uint8_t)((i-1) & 0xFF));
    }
}

void QGCUASFileManagerUnitTest::_openTest(void)
{
    Q_ASSERT(_fileManager);
    Q_ASSERT(_multiSpy);
    Q_ASSERT(_multiSpy->checkNoSignals() == true);
    
    // Send a bogus path
    //  We should get a single resetStatusMessages signal
    //  We should get a single errorMessage signal
    _fileManager->downloadPath("bogus", QDir::temp());
    QCOMPARE(_multiSpy->checkOnlySignalByMask(errorMessageSignalMask | resetStatusMessagesSignalMask), true);
    _multiSpy->clearAllSignals();
    
    // Clean previous downloads
    for (size_t i=0; i<MockMavlinkFileServer::cFileTestCases; i++) {
        QString filePath = QDir::temp().absoluteFilePath(MockMavlinkFileServer::rgFileTestCases[i].filename);
        if (QFile::exists(filePath)) {
            Q_ASSERT(QFile::remove(filePath));
        }
    }

    // Run through the set of file test cases
    
    // We setup a spy on the terminate command signal so that we can determine that a Terminate command was
    // correctly sent after the Open/Read commands complete.
    QSignalSpy terminateSpy(&_mockFileServer, SIGNAL(terminateCommandReceived()));
    
    for (size_t i=0; i<MockMavlinkFileServer::cFileTestCases; i++) {
        _fileManager->downloadPath(MockMavlinkFileServer::rgFileTestCases[i].filename, QDir::temp());

        //  We should get a single resetStatusMessages signal
        //  We should get a single statusMessage signal, which indicated download completion
        QCOMPARE(_multiSpy->checkOnlySignalByMask(statusMessageSignalMask | resetStatusMessagesSignalMask), true);
        _multiSpy->clearAllSignals();
        
        // We should get a single Terminate command
        QCOMPARE(terminateSpy.count(), 1);
        terminateSpy.clear();
        
        QString filePath = QDir::temp().absoluteFilePath(MockMavlinkFileServer::rgFileTestCases[i].filename);
        _validateFileContents(filePath, MockMavlinkFileServer::rgFileTestCases[i].length);
    }
}
