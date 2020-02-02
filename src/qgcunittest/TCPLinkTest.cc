/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @brief TCPLink class unit test
///
///     @author Don Gagne <don@thegagnes.com>

#include "TCPLinkTest.h"
#include "TCPLoopBackServer.h"

TCPLinkTest::TCPLinkTest(void)
    : _link(nullptr)
    , _multiSpy(nullptr)
{

}

// Called before every test
void TCPLinkTest::init(void)
{
    UnitTest::init();
    
    Q_ASSERT(_link == nullptr);
    Q_ASSERT(_multiSpy == nullptr);

    TCPConfiguration* tcpConfig = new TCPConfiguration("MockTCP");
    tcpConfig->setAddress(QHostAddress::LocalHost);
    tcpConfig->setPort(5760);
    _sharedConfig = SharedLinkConfigurationPointer(tcpConfig);
    _link = new TCPLink(_sharedConfig);

    _rgSignals[bytesReceivedSignalIndex] = SIGNAL(bytesReceived(LinkInterface*, QByteArray));
    _rgSignals[connectedSignalIndex] = SIGNAL(connected(void));
    _rgSignals[disconnectedSignalIndex] = SIGNAL(disconnected(void));
    _rgSignals[communicationErrorSignalIndex] = SIGNAL(communicationError(const QString&, const QString&));
    _rgSignals[communicationUpdateSignalIndex] = SIGNAL(communicationUpdate(const QString&, const QString&));
    //_rgSignals[deleteLinkSignalIndex] = SIGNAL(_deleteLink(LinkInterface*));

    _multiSpy = new MultiSignalSpy();
    QCOMPARE(_multiSpy->init(_link, _rgSignals, _cSignals), true);
}

// Called after every test
void TCPLinkTest::cleanup(void)
{
    Q_ASSERT(_multiSpy);
    Q_ASSERT(_link);

    delete _multiSpy;
    _multiSpy = nullptr;

    delete _link;
    _link = nullptr;

    _sharedConfig.clear();

    UnitTest::cleanup();
}

void TCPLinkTest::_connectFail_test(void)
{
    Q_ASSERT(_link);
    Q_ASSERT(_multiSpy);
    Q_ASSERT(_multiSpy->checkNoSignals() == true);
    
    // With the new threading model connect will always succeed. We only get an error signal
    // for a failed connected.
    QCOMPARE(_link->_connect(), true);

    // Make sure we get a linkError signal with the right link name
    QCOMPARE(_multiSpy->waitForSignalByIndex(communicationErrorSignalIndex, 1000), true);
    QCOMPARE(_multiSpy->checkOnlySignalByMask(communicationErrorSignalMask), true);
    QList<QVariant> arguments = _multiSpy->getSpyByIndex(communicationErrorSignalIndex)->takeFirst();
    QCOMPARE(arguments.at(1).toString().contains(_link->getName()), true);
    _multiSpy->clearSignalByIndex(communicationErrorSignalIndex);
    
    _link->_disconnect();

    // Try to connect again to make sure everything was cleaned up correctly from previous failed connection
    
    QCOMPARE(_link->_connect(), true);
    
    // Make sure we get a linkError signal with the right link name
    QCOMPARE(_multiSpy->waitForSignalByIndex(communicationErrorSignalIndex, 1000), true);
    QCOMPARE(_multiSpy->checkOnlySignalByMask(communicationErrorSignalMask), true);
    arguments = _multiSpy->getSpyByIndex(communicationErrorSignalIndex)->takeFirst();
    QCOMPARE(arguments.at(1).toString().contains(_link->getName()), true);
    _multiSpy->clearSignalByIndex(communicationErrorSignalIndex);
}

void TCPLinkTest::_connectSucceed_test(void)
{
    QSKIP("FIXME: Failing on OSX");

    Q_ASSERT(_link);
    Q_ASSERT(_multiSpy);
    Q_ASSERT(_multiSpy->checkNoSignals() == true);

    // Start the server side
    TCPConfiguration* tcpConfig = qobject_cast<TCPConfiguration*>(_sharedConfig.data());
    TCPLoopBackServer* server = new TCPLoopBackServer(tcpConfig->address(), tcpConfig->port());
    Q_CHECK_PTR(server);
    
    // Connect to the server
    QCOMPARE(_link->_connect(), true);
    
    // Make sure we get the connected signals
    QCOMPARE(_multiSpy->waitForSignalByIndex(connectedSignalIndex, 10000), true);
    QCOMPARE(_multiSpy->checkOnlySignalByMask(connectedSignalMask), true);
    _multiSpy->clearAllSignals();
    
    // Test link->server data path
    
    QByteArray bytesOut("test");

    // Write data from link to server
    const char* bytesWrittenSignal = SIGNAL(bytesWritten(qint64));
    MultiSignalSpy bytesWrittenSpy;
    QCOMPARE(bytesWrittenSpy.init(_link->getSocket(), &bytesWrittenSignal, 1), true);
    _link->writeBytesSafe(bytesOut.data(), bytesOut.size());
    _multiSpy->clearAllSignals();
    
    // We emit this signal such that it will be queued and run on the TCPLink thread. This in turn
    // allows the TCPLink object to pump the bytes through.
    connect(this, &TCPLinkTest::waitForBytesWritten, _link, &TCPLink::waitForBytesWritten);
    emit waitForBytesWritten(1000);

    // Check for loopback, both from signal received and actual bytes returned

    QCOMPARE(_multiSpy->waitForSignalByIndex(bytesReceivedSignalIndex, 1000), true);
    QCOMPARE(_multiSpy->checkOnlySignalByMask(bytesReceivedSignalMask), true);
    
    // Read the data and make sure it matches
    QList<QVariant> arguments = _multiSpy->getSpyByIndex(bytesReceivedSignalIndex)->takeFirst();
    QVERIFY(arguments.at(1).toByteArray() == bytesOut);
    
    _multiSpy->clearAllSignals();

    // Disconnect the link
    _link->_disconnect();
    
    // Make sure we get the disconnected signal on link side
    QCOMPARE(_multiSpy->waitForSignalByIndex(disconnectedSignalIndex, 1000), true);
    QCOMPARE(_multiSpy->checkOnlySignalByMask(disconnectedSignalMask), true);
    _multiSpy->clearAllSignals();
    
    // Try to connect again to make sure everything was cleaned up correctly from previous connection
    
    // Connect to the server
    QCOMPARE(_link->_connect(), true);
    
    // Make sure we get the connected signal
    QCOMPARE(_multiSpy->waitForSignalByIndex(connectedSignalIndex, 1000), true);
    QCOMPARE(_multiSpy->checkOnlySignalByMask(connectedSignalMask), true);
    _multiSpy->clearAllSignals();
    
    server->quit();
    QTest::qWait(500);  // Wait a little for server thread to terminate
    delete server;
}
