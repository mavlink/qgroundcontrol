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

#include "FileManagerTest.h"
#include "MultiVehicleManager.h"
#include "UAS.h"
#include "QGCApplication.h"

FileManagerTest::FileManagerTest(void)
    : _fileServer(NULL)
    , _fileManager(NULL)
    , _multiSpy(NULL)
{

}

// Called before every test case
void FileManagerTest::init(void)
{
    UnitTest::init();
    
    _connectMockLink();

    _fileServer = _mockLink->getFileServer();
    QVERIFY(_fileServer != NULL);
    
    _fileManager = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle()->uas()->getFileManager();
    QVERIFY(_fileManager != NULL);
    
    Q_ASSERT(_multiSpy == NULL);
    
    // Reset any internal state back to normal
    _fileServer->setErrorMode(MockLinkFileServer::errModeNone);
    _fileListReceived.clear();

    connect(_fileManager, &FileManager::listEntry, this, &FileManagerTest::listEntry);

    _rgSignals[listEntrySignalIndex] = SIGNAL(listEntry(const QString&));
    _rgSignals[commandCompleteSignalIndex] = SIGNAL(commandComplete(void));
    _rgSignals[commandErrorSignalIndex] = SIGNAL(commandError(const QString&));

    _multiSpy = new MultiSignalSpy();
    Q_CHECK_PTR(_multiSpy);
    QCOMPARE(_multiSpy->init(_fileManager, _rgSignals, _cSignals), true);
}

// Called after every test case
void FileManagerTest::cleanup(void)
{
    Q_ASSERT(_multiSpy);
    Q_ASSERT(_fileManager);
    
    // Disconnecting the link will prompt for log file save
    setExpectedFileDialog(getSaveFileName, QStringList());
    _disconnectMockLink();

    _fileServer = NULL;
    _fileManager = NULL;
    
    delete _multiSpy;
    
    _multiSpy = NULL;
    
    UnitTest::cleanup();
}

/// @brief Connected to FileManager listEntry signal in order to catch list entries
void FileManagerTest::listEntry(const QString& entry)
{
    // Keep a list of all names received so we can test it for correctness
    _fileListReceived += entry;
}


void FileManagerTest::_ackTest(void)
{
    Q_ASSERT(_fileManager);
    Q_ASSERT(_multiSpy);
    Q_ASSERT(_multiSpy->checkNoSignals() == true);
    
    // If the file manager doesn't receive an ack it will timeout and emit an error. So make sure
    // we don't get any error signals.
    //TODO: FIX
    //QVERIFY(_fileManager->_sendCmdTestAck());
    //QTest::qWait(_ackTimerTimeoutMsecs); // Let the file manager timeout
    //QVERIFY(_multiSpy->checkNoSignals());
    
    // Setup for no response from ack. This should cause a timeout error
    _fileServer->setErrorMode(MockLinkFileServer::errModeNoResponse);
    QVERIFY(_fileManager->_sendCmdTestAck());
    _multiSpy->waitForSignalByIndex(commandErrorSignalIndex, _ackTimerTimeoutMsecs);
    QCOMPARE(_multiSpy->checkOnlySignalByMask(commandErrorSignalMask), true);
    _multiSpy->clearAllSignals();

    // Setup for a bad sequence number in the ack. This should cause an error;
    _fileServer->setErrorMode(MockLinkFileServer::errModeBadSequence);
    QVERIFY(_fileManager->_sendCmdTestAck());
    _multiSpy->waitForSignalByIndex(commandErrorSignalIndex, _ackTimerTimeoutMsecs);
    QCOMPARE(_multiSpy->checkOnlySignalByMask(commandErrorSignalMask), true);
    _multiSpy->clearAllSignals();
}

void FileManagerTest::_noAckTest(void)
{
    Q_ASSERT(_fileManager);
    Q_ASSERT(_multiSpy);
    Q_ASSERT(_multiSpy->checkNoSignals() == true);
    
    // This should not get the ack back and timeout.
    QVERIFY(_fileManager->_sendCmdTestNoAck());
    QTest::qWait(_ackTimerTimeoutMsecs); // Let the file manager timeout
    QCOMPARE(_multiSpy->checkOnlySignalByMask(commandErrorSignalMask), true);
}

void FileManagerTest::_listTest(void)
{
    Q_ASSERT(_fileManager);
    Q_ASSERT(_multiSpy);
    Q_ASSERT(_multiSpy->checkNoSignals() == true);

    // test the automatic retry behavior by enabling random drops
    _fileServer->enableRandromDrops(true);
    
    // FileManager::listDirectory signalling as follows:
    //  Emits a listEntry signal for each list entry
    //  Emits an commandError signal if:
    //      It gets a Nak back
    //      Sequence number is incorrrect on any response
    //      CRC is incorrect on any responses
    //      List entry is formatted incorrectly
    //  It is possible to get a number of good listEntry signals, followed by an commandError signal
    //  Emits commandComplete after it receives the final list entry
    //      If an commandError signal is signalled no listComplete is signalled
    
    // Send a bogus path
    //  We should get a single commandError signal
    _fileManager->listDirectory("/bogus");
    _multiSpy->waitForSignalByIndex(commandErrorSignalIndex, _ackTimerTimeoutMsecs);
    QCOMPARE(_multiSpy->checkOnlySignalByMask(commandErrorSignalMask), true);
    _multiSpy->clearAllSignals();

    // Setup the mock file server with a valid directory list
    QStringList fileList;
    fileList << "Ddir" << "Ffoo" << "Fbar";
    _fileServer->setFileList(fileList);
    
    // Send a list command at the root of the directory tree which should succeed
    _fileManager->listDirectory("/");
    QTest::qWait(_ackTimerTimeoutMsecs); // Let the file manager timeout
    QCOMPARE(_multiSpy->checkSignalByMask(commandCompleteSignalMask), true);
    QCOMPARE(_multiSpy->checkNoSignalByMask(commandErrorSignalMask), true);
    QCOMPARE(_multiSpy->getSpyByIndex(listEntrySignalIndex)->count(), fileList.count());
    QVERIFY(_fileListReceived == fileList);
    
    // Set everything back to initial state
    _fileListReceived.clear();
    _multiSpy->clearAllSignals();
    
    // Run through the various server side failure modes
    for (size_t i=0; i<MockLinkFileServer::cFailureModes; i++) {
        MockLinkFileServer::ErrorMode_t errMode = MockLinkFileServer::rgFailureModes[i];
        qDebug() << "Testing failure mode:" << errMode;
        _fileServer->setErrorMode(errMode);
        
        _fileManager->listDirectory("/");
        QTest::qWait(_ackTimerTimeoutMsecs); // Let the file manager timeout
        
        if (errMode == MockLinkFileServer::errModeNoSecondResponse || errMode == MockLinkFileServer::errModeNakSecondResponse) {
            // For simulated server errors on subsequent Acks, the first Ack will go through. This means we should have gotten some
            // partial results. In the case of the directory list test set, all entries fit into the first ack, so we should have
            // gotten back all of them.
            QCOMPARE(_multiSpy->getSpyByIndex(listEntrySignalIndex)->count(), fileList.count());
            _multiSpy->clearSignalByIndex(listEntrySignalIndex);
            
            // And then it should have errored out because the next list Request would have failed.
            QCOMPARE(_multiSpy->checkOnlySignalByMask(commandErrorSignalMask), true);
        } else {
            // For the simulated errors which failed the initial response we should not have gotten any results back at all.
            // Just an error.
            QCOMPARE(_multiSpy->checkOnlySignalByMask(commandErrorSignalMask), true);
        }

        // Set everything back to initial state
        _fileListReceived.clear();
        _multiSpy->clearAllSignals();
        _fileServer->setErrorMode(MockLinkFileServer::errModeNone);
    }

    _fileServer->enableRandromDrops(false);
}

#if 0
// Trying to write test code for read and burst mode download as well as implement support in MockLineFileServer reached a point
// of diminishing returns where the test code and mock server were generating more bugs in themselves than finding problems.
void FileManagerTest::_readDownloadTest(void)
{
    Q_ASSERT(_fileManager);
    Q_ASSERT(_multiSpy);
    Q_ASSERT(_multiSpy->checkNoSignals() == true);
    
    // We setup a spy on the Reset command signal of the mock file server so that we can determine that a
    // Reset command was correctly sent after the Open/Read commands complete.
    QSignalSpy resetSpy(_fileServer, SIGNAL(resetCommandReceived()));
    
    // Send a bogus path
    _fileManager->downloadPath("bogus", QDir::temp());
    _multiSpy->waitForSignalByIndex(commandErrorSignalIndex, _ackTimerTimeoutMsecs);
    QCOMPARE(_multiSpy->checkOnlySignalByMask(commandErrorSignalMask), true);
    _multiSpy->clearAllSignals();
    QCOMPARE(resetSpy.count(), 0);

    // Clean previous downloads
    for (size_t i=0; i<MockLinkFileServer::cFileTestCases; i++) {
        QString filePath = QDir::temp().absoluteFilePath(MockLinkFileServer::rgFileTestCases[i].filename);
        if (QFile::exists(filePath)) {
            Q_ASSERT(QFile::remove(filePath));
        }
    }
    
    // Run through the set of file test cases
    for (size_t i=0; i<MockLinkFileServer::cFileTestCases; i++) {
        const MockLinkFileServer::FileTestCase* testCase = &MockLinkFileServer::rgFileTestCases[i];
        
        // Run through the various failure modes for this test case
        for (size_t j=0; j<MockLinkFileServer::cFailureModes; j++) {
            qDebug() << "Testing successful download";
			
            // Run what should be a successful file download test case. No servers errors are being simulated.
            _fileServer->setErrorMode(MockLinkFileServer::errModeNone);
            _fileManager->downloadPath(testCase->filename, QDir::temp());
            QTest::qWait(_ackTimerTimeoutMsecs); // Let the file manager timeout
            
            // This should be a successful download
            QCOMPARE(_multiSpy->checkOnlySignalByMask(commandCompleteSignalMask), true);
            _multiSpy->clearAllSignals();
            
            // We should get a single Reset command to close the session
            QCOMPARE(resetSpy.count(), 1);
            resetSpy.clear();
            
            // Validate file contents
            QString filePath = QDir::temp().absoluteFilePath(MockLinkFileServer::rgFileTestCases[i].filename);
            _validateFileContents(filePath, MockLinkFileServer::rgFileTestCases[i].length);
            MockLinkFileServer::ErrorMode_t errMode = MockLinkFileServer::rgFailureModes[j];
            
            qDebug() << "Testing failure mode:" << errMode;
            _fileServer->setErrorMode(errMode);
            
            _fileManager->downloadPath(testCase->filename, QDir::temp());
            QTest::qWait(_ackTimerTimeoutMsecs); // Let the file manager timeout
            
            if (errMode == MockLinkFileServer::errModeNakResponse) {
                // This will Nak the Open call which will fail the download, but not cause a Reset
                QCOMPARE(_multiSpy->checkOnlySignalByMask(commandErrorSignalMask), true);
                QCOMPARE(resetSpy.count(), 0);
            } else {
                if (testCase->packetCount == 1 && (errMode == MockLinkFileServer::errModeNoSecondResponse || errMode == MockLinkFileServer::errModeNakSecondResponse)) {
                    // The downloaded file fits within a single Ack response, hence there is no second Read issued.
                    // This should result in a successful download, followed by a Reset
                    QCOMPARE(_multiSpy->checkOnlySignalByMask(commandCompleteSignalMask), true);
                    QCOMPARE(resetSpy.count(), 1);
                    
                    // Validate file contents
                    QString filePath = QDir::temp().absoluteFilePath(testCase->filename);
                    _validateFileContents(filePath, testCase->length);
                } else {
                    // Download should have failed, followed by a aReset
                    QCOMPARE(_multiSpy->checkOnlySignalByMask(commandErrorSignalMask), true);
                    QCOMPARE(resetSpy.count(), 1);
                }
            }

            // Cleanup for next iteration
            _multiSpy->clearAllSignals();
            resetSpy.clear();
            _fileServer->setErrorMode(MockLinkFileServer::errModeNone);
        }
    }
}

void FileManagerTest::_streamDownloadTest(void)
{
    Q_ASSERT(_fileManager);
    Q_ASSERT(_multiSpy);
    Q_ASSERT(_multiSpy->checkNoSignals() == true);
    
    // We setup a spy on the Reset command signal of the mock file server so that we can determine that a
    // Reset command was correctly sent after the Open/Read commands complete.
    QSignalSpy resetSpy(_fileServer, SIGNAL(resetCommandReceived()));
    
    // Send a bogus path
    _fileManager->streamPath("bogus", QDir::temp());
    _multiSpy->waitForSignalByIndex(commandErrorSignalIndex, _ackTimerTimeoutMsecs);
    QCOMPARE(_multiSpy->checkOnlySignalByMask(commandErrorSignalMask), true);
    _multiSpy->clearAllSignals();
    QCOMPARE(resetSpy.count(), 0);

    // Clean previous downloads
    for (size_t i=0; i<MockLinkFileServer::cFileTestCases; i++) {
        QString filePath = QDir::temp().absoluteFilePath(MockLinkFileServer::rgFileTestCases[i].filename);
        if (QFile::exists(filePath)) {
            Q_ASSERT(QFile::remove(filePath));
        }
    }
    
    // Run through the set of file test cases
    for (size_t i=0; i<MockLinkFileServer::cFileTestCases; i++) {
        const MockLinkFileServer::FileTestCase* testCase = &MockLinkFileServer::rgFileTestCases[i];
        
        // Run through the various failure modes for this test case
        for (size_t j=0; j<MockLinkFileServer::cFailureModes; j++) {
            qDebug() << "Testing successful download";
			
            // Run what should be a successful file download test case. No servers errors are being simulated.
            _fileServer->setErrorMode(MockLinkFileServer::errModeNone);
            _fileManager->streamPath(testCase->filename, QDir::temp());
            QTest::qWait(_ackTimerTimeoutMsecs); // Let the file manager timeout
            
            // This should be a successful download
            QCOMPARE(_multiSpy->checkOnlySignalByMask(commandCompleteSignalMask), true);
            _multiSpy->clearAllSignals();
            
            // We should get a single Reset command to close the session
            QCOMPARE(resetSpy.count(), 1);
            resetSpy.clear();
            
            // Validate file contents
            QString filePath = QDir::temp().absoluteFilePath(MockLinkFileServer::rgFileTestCases[i].filename);
            _validateFileContents(filePath, MockLinkFileServer::rgFileTestCases[i].length);
            MockLinkFileServer::ErrorMode_t errMode = MockLinkFileServer::rgFailureModes[j];
            
            qDebug() << "Testing failure mode:" << errMode;
            _fileServer->setErrorMode(errMode);
            
            _fileManager->downloadPath(testCase->filename, QDir::temp());
            QTest::qWait(_ackTimerTimeoutMsecs); // Let the file manager timeout
            
            if (errMode == MockLinkFileServer::errModeNakResponse) {
                // This will Nak the Open call which will fail the download, but not cause a Reset
                QCOMPARE(_multiSpy->checkOnlySignalByMask(commandErrorSignalMask), true);
                QCOMPARE(resetSpy.count(), 0);
            } else {
                if (testCase->packetCount == 1 && (errMode == MockLinkFileServer::errModeNoSecondResponse || errMode == MockLinkFileServer::errModeNakSecondResponse)) {
                    // The downloaded file fits within a single Ack response, hence there is no second Read issued.
                    // This should result in a successful download, followed by a Reset
                    QCOMPARE(_multiSpy->checkOnlySignalByMask(commandCompleteSignalMask), true);
                    QCOMPARE(resetSpy.count(), 1);
                    
                    // Validate file contents
                    QString filePath = QDir::temp().absoluteFilePath(testCase->filename);
                    _validateFileContents(filePath, testCase->length);
                } else {
                    // Download should have failed, followed by a aReset
                    QCOMPARE(_multiSpy->checkOnlySignalByMask(commandErrorSignalMask), true);
                    QCOMPARE(resetSpy.count(), 1);
                }
            }

            // Cleanup for next iteration
            _multiSpy->clearAllSignals();
            resetSpy.clear();
            _fileServer->setErrorMode(MockLinkFileServer::errModeNone);
        }
    }
}

void FileManagerTest::_validateFileContents(const QString& filePath, uint8_t length)
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
#endif
