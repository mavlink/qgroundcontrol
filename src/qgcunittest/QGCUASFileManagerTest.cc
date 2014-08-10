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
    
    // Reset any internal state back to normal
    _mockFileServer.setErrorMode(MockMavlinkFileServer::errModeNone);
    _fileListReceived.clear();
    
    bool connected = connect(&_mockFileServer, SIGNAL(messageReceived(LinkInterface*, mavlink_message_t)), _fileManager, SLOT(receiveMessage(LinkInterface*, mavlink_message_t)));
    Q_ASSERT(connected);
    Q_UNUSED(connected);    // Silent release build compiler warning

    connected = connect(_fileManager, SIGNAL(statusMessage(const QString&)), this, SLOT(statusMessage(const QString&)));
    Q_ASSERT(connected);

    _rgSignals[statusMessageSignalIndex] = SIGNAL(statusMessage(const QString&));
    _rgSignals[errorMessageSignalIndex] = SIGNAL(errorMessage(const QString&));
    _rgSignals[resetStatusMessagesSignalIndex] = SIGNAL(resetStatusMessages(void));
    _rgSignals[listCompleteSignalIndex] = SIGNAL(listComplete(void));
    _rgSignals[openFileLengthSignalIndex] = SIGNAL(openFileLength(unsigned int));
    
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
    QTest::qWait(_ackTimerTimeoutMsecs); // Let the file manager timeout
    QVERIFY(_multiSpy->checkNoSignals());
    
    // Setup for no response from ack. This should cause a timeout error
    _mockFileServer.setErrorMode(MockMavlinkFileServer::errModeNoResponse);
    QVERIFY(_fileManager->_sendCmdTestAck());
    QTest::qWait(_ackTimerTimeoutMsecs); // Let the file manager timeout
    QCOMPARE(_multiSpy->checkOnlySignalByMask(errorMessageSignalMask), true);
    _multiSpy->clearAllSignals();

    // Setup for a bad sequence number in the ack. This should cause an error;
    _mockFileServer.setErrorMode(MockMavlinkFileServer::errModeBadSequence);
    QVERIFY(_fileManager->_sendCmdTestAck());
    QCOMPARE(_multiSpy->checkOnlySignalByMask(errorMessageSignalMask), true);
    _multiSpy->clearAllSignals();
}

void QGCUASFileManagerUnitTest::_noAckTest(void)
{
    Q_ASSERT(_fileManager);
    Q_ASSERT(_multiSpy);
    Q_ASSERT(_multiSpy->checkNoSignals() == true);
    
    // This should not get the ack back and timeout.
    QVERIFY(_fileManager->_sendCmdTestNoAck());
    QTest::qWait(_ackTimerTimeoutMsecs); // Let the file manager timeout
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
    
    // QGCUASFileManager::listDirectory signalling as follows:
    //  Emits the resetStatusMessages signal
    //  Emits a statusMessage signal for each list entry
    //  Emits an errorMessage signal if:
    //      It gets a Nak back
    //      Sequence number is incorrrect on any response
    //      CRC is incorrect on any responses
    //      List entry is formatted incorrectly
    //  It is possible to get a number of good statusMessage signals, followed by an errorMessage signal
    //  Emits listComplete after it receives the final list entry
    //      If an errorMessage signal is signalled no listComplete is signalled
    
    // Send a bogus path
    //  We should get a single resetStatusMessages signal
    //  We should get a single errorMessage signal
    _fileManager->listDirectory("/bogus");
    QCOMPARE(_multiSpy->checkOnlySignalByMask(errorMessageSignalMask | resetStatusMessagesSignalMask), true);
    _multiSpy->clearAllSignals();

    // Setup the mock file server with a valid directory list
    QStringList fileList;
    fileList << "Ddir" << "Ffoo" << "Fbar";
    _mockFileServer.setFileList(fileList);
    
    // Run through the various server side failure modes
    for (size_t i=0; i<MockMavlinkFileServer::cFailureModes; i++) {
        MockMavlinkFileServer::ErrorMode_t errMode = MockMavlinkFileServer::rgFailureModes[i];
        qDebug() << "Testing failure mode:" << errMode;
        _mockFileServer.setErrorMode(errMode);
        
        _fileManager->listDirectory("/");
        QTest::qWait(_ackTimerTimeoutMsecs); // Let the file manager timeout
        
        if (errMode == MockMavlinkFileServer::errModeNoSecondResponse || errMode == MockMavlinkFileServer::errModeNakSecondResponse) {
            // For simulated server errors on subsequent Acks, the first Ack will go through. This means we should have gotten some
            // partial results. In the case of the directory list test set, all entries fit into the first ack, so we should have
            // gotten back all of them.
            QCOMPARE(_multiSpy->getSpyByIndex(statusMessageSignalIndex)->count(), fileList.count());
            _multiSpy->clearSignalByIndex(statusMessageSignalIndex);
            
            // And then it should have errored out because the next list Request would have failed.
            QCOMPARE(_multiSpy->checkOnlySignalByMask(errorMessageSignalMask | resetStatusMessagesSignalMask), true);
        } else {
            // For the simulated errors which failed the intial response we should not have gotten any results back at all.
            // Just an error.
            QCOMPARE(_multiSpy->checkOnlySignalByMask(errorMessageSignalMask | resetStatusMessagesSignalMask), true);
        }

        // Set everything back to initial state
        _fileListReceived.clear();
        _multiSpy->clearAllSignals();
        _mockFileServer.setErrorMode(MockMavlinkFileServer::errModeNone);
    }

    // Send a list command at the root of the directory tree which should succeed    
    _fileManager->listDirectory("/");
    QCOMPARE(_multiSpy->checkSignalByMask(resetStatusMessagesSignalMask | listCompleteSignalMask), true);
    QCOMPARE(_multiSpy->checkNoSignalByMask(errorMessageSignalMask), true);
    QCOMPARE(_multiSpy->getSpyByIndex(statusMessageSignalIndex)->count(), fileList.count());
    QVERIFY(_fileListReceived == fileList);
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
    
    // Validate file contents:
    //      Repeating 0x00, 0x01 .. 0xFF until file is full
    for (uint8_t i=0; i<bytes.length(); i++) {
        QCOMPARE((uint8_t)bytes[i], (uint8_t)(i & 0xFF));
    }
}

void QGCUASFileManagerUnitTest::_downloadTest(void)
{
    Q_ASSERT(_fileManager);
    Q_ASSERT(_multiSpy);
    Q_ASSERT(_multiSpy->checkNoSignals() == true);
    
    // QGCUASFileManager::downloadPath works as follows:
    //  Emits the resetStatusMessages signal
    //  Sends an Open Command to the server
    //      Expects an Ack Response back from the server with the correct sequence numner
    //          Emits an errorMessage signal if it gets a Nak back
    //      Emits an openFileLength signal with the file length if it gets back a good Ack
    //  Sends subsequent Read commands to the server until it gets the full file contents back
    //  Sends Terminate command to server when download is complete to close Open command
    //      Mock file server will signal terminateCommandReceived when it gets a Terminate command
    //  Sends statusMessage signal to indicate the download is complete
    //  Emits an errorMessage signal if sequence number is incorrrect on any response
    //  Emits an errorMessage signal if CRC is incorrect on any responses
    
    // Expected signals if the Open command fails for any reason
    quint16 signalMaskOpenFailure = resetStatusMessagesSignalMask | errorMessageSignalMask;

    // Expected signals if the Read command fails for any reason
    quint16 signalMaskReadFailure = resetStatusMessagesSignalMask | openFileLengthSignalMask | errorMessageSignalMask;
    
    // Expected signals if the downloadPath command succeeds
    quint16 signalMaskDownloadSuccess = resetStatusMessagesSignalMask | openFileLengthSignalMask | statusMessageSignalMask;

    // Send a bogus path
    //  We should get a single resetStatusMessages signal
    //  We should get a single errorMessage signal
    _fileManager->downloadPath("bogus", QDir::temp());
    QCOMPARE(_multiSpy->checkOnlySignalByMask(signalMaskOpenFailure), true);
    _multiSpy->clearAllSignals();
    
    // Clean previous downloads
    for (size_t i=0; i<MockMavlinkFileServer::cFileTestCases; i++) {
        QString filePath = QDir::temp().absoluteFilePath(MockMavlinkFileServer::rgFileTestCases[i].filename);
        if (QFile::exists(filePath)) {
            Q_ASSERT(QFile::remove(filePath));
        }
    }
    
    // We setup a spy on the Terminate command signal of the mock file server so that we can determine that a
    // Terminate command was correctly sent after the Open/Read commands complete.
    QSignalSpy terminateSpy(&_mockFileServer, SIGNAL(terminateCommandReceived()));
    
    // Run through the set of file test cases
    for (size_t i=0; i<MockMavlinkFileServer::cFileTestCases; i++) {
        const MockMavlinkFileServer::FileTestCase* testCase = &MockMavlinkFileServer::rgFileTestCases[i];
        
        // Run through the various failure modes for this test case
        for (size_t j=0; j<MockMavlinkFileServer::cFailureModes; j++) {
            
            MockMavlinkFileServer::ErrorMode_t errMode = MockMavlinkFileServer::rgFailureModes[j];
            qDebug() << "Testing failure mode:" << errMode;
            _mockFileServer.setErrorMode(errMode);
            
            _fileManager->downloadPath(testCase->filename, QDir::temp());
            QTest::qWait(_ackTimerTimeoutMsecs); // Let the file manager timeout
            
            if (errMode == MockMavlinkFileServer::errModeNoSecondResponse || errMode == MockMavlinkFileServer::errModeNakSecondResponse) {
                // For simulated server errors on subsequent Acks, the first Ack will go through. We must handle things differently depending
                // on whether the downloaded file requires multiple packets to complete the download.
                if (testCase->fMultiPacketResponse) {
                    // The downloaded file requires multiple Acks to complete. Hence second Read should have failed.
                    QCOMPARE(_multiSpy->checkOnlySignalByMask(signalMaskReadFailure), true);

                    // Open command succeeded, so we should get a Terminate for the open
                    QCOMPARE(terminateSpy.count(), 1);
                } else {
                    // The downloaded file fits within a single Ack response, hence there is no second Read issued.
                    // This should result in a successful download.
                    QCOMPARE(_multiSpy->checkOnlySignalByMask(signalMaskDownloadSuccess), true);
                    
                    // We should get a single Terminate command to close the Open session
                    QCOMPARE(terminateSpy.count(), 1);
                    
                    // Validate file contents
                    QString filePath = QDir::temp().absoluteFilePath(testCase->filename);
                    _validateFileContents(filePath, testCase->length);
                }
            } else {
                // For all the other simulated server errors the Open command should have failed. Since the Open failed
                // there is no session to terminate, hence no Terminate in this case.
                QCOMPARE(_multiSpy->checkOnlySignalByMask(signalMaskOpenFailure), true);
                QCOMPARE(terminateSpy.count(), 0);
            }

            // Cleanup for next iteration
            _multiSpy->clearAllSignals();
            terminateSpy.clear();
            _mockFileServer.setErrorMode(MockMavlinkFileServer::errModeNone);
        }

        // Run what should be a successful file download test case. No servers errors are being simulated.
        _fileManager->downloadPath(testCase->filename, QDir::temp());

        // This should be a succesful download
        QCOMPARE(_multiSpy->checkOnlySignalByMask(signalMaskDownloadSuccess), true);
        
        // Make sure the file length coming back through the openFileLength signal is correct
        QVERIFY(_multiSpy->getSpyByIndex(openFileLengthSignalIndex)->takeFirst().at(0).toInt() == testCase->length);

        _multiSpy->clearAllSignals();
        
        // We should get a single Terminate command to close the session
        QCOMPARE(terminateSpy.count(), 1);
        terminateSpy.clear();
        
        // Validate file contents
        QString filePath = QDir::temp().absoluteFilePath(MockMavlinkFileServer::rgFileTestCases[i].filename);
        _validateFileContents(filePath, MockMavlinkFileServer::rgFileTestCases[i].length);
    }
}
