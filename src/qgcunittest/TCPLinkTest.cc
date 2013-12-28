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

#include "TCPLinkTest.h"
#include <QTcpServer>

/// @file
///     @brief TCPLink class unit test
///
///     @author Don Gagne <don@thegagnes.com>

TCPLinkUnitTest::TCPLinkUnitTest(void) :
    _link(NULL),
    _hostAddress(QHostAddress::LocalHost),
    _port(5760),
    _multiSpy(NULL)
{
    
}

// Called before every test
void TCPLinkUnitTest::init(void)
{
    Q_ASSERT(_link == NULL);
    Q_ASSERT(_multiSpy == NULL);

    _link = new TCPLink(_hostAddress, _port);
    Q_ASSERT(_link != NULL);

    _rgSignals[bytesReceivedSignalIndex] = SIGNAL(bytesReceived(LinkInterface*, QByteArray));
    _rgSignals[connectedSignalIndex] = SIGNAL(connected(void));
    _rgSignals[disconnectedSignalIndex] = SIGNAL(disconnected(void));
    _rgSignals[connected2SignalIndex] = SIGNAL(connected(bool));
    _rgSignals[nameChangedSignalIndex] = SIGNAL(nameChanged(QString));
    _rgSignals[communicationErrorSignalIndex] = SIGNAL(communicationError(const QString&, const QString&));
    _rgSignals[communicationUpdateSignalIndex] = SIGNAL(communicationUpdate(const QString&, const QString&));
    _rgSignals[deleteLinkSignalIndex] = SIGNAL(deleteLink(LinkInterface* const));

    _multiSpy = new MultiSignalSpy();
    QCOMPARE(_multiSpy->init(_link, _rgSignals, _cSignals), true);
}

// Called after every test
void TCPLinkUnitTest::cleanup(void)
{
    Q_ASSERT(_multiSpy);
    Q_ASSERT(_link);

    delete _multiSpy;
    delete _link;

    _multiSpy = NULL;
    _link = NULL;
}

void TCPLinkUnitTest::_properties_test(void)
{
    Q_ASSERT(_link);
    Q_ASSERT(_multiSpy);
    Q_ASSERT(_multiSpy->checkNoSignals() == true);
    
    // Make sure we get the right values back
    QHostAddress    hostAddressOut;
    quint16         portOut;
    
    hostAddressOut = _link->getHostAddress();
    QCOMPARE(_hostAddress, hostAddressOut);

    portOut = _link->getPort();
    QCOMPARE(_port, portOut);
}

void TCPLinkUnitTest::_nameChangedSignal_test(void)
{
    Q_ASSERT(_link);
    Q_ASSERT(_multiSpy);
    Q_ASSERT(_multiSpy->checkNoSignals() == true);
    
    _link->setHostAddress(QHostAddress("127.1.1.1"));
    QCOMPARE(_multiSpy->checkOnlySignalByMask(nameChangedSignalMask), true);
    _multiSpy->clearSignalByIndex(nameChangedSignalIndex);
    
    _link->setPort(42);
    QCOMPARE(_multiSpy->checkOnlySignalByMask(nameChangedSignalMask), true);
    _multiSpy->clearSignalByIndex(nameChangedSignalIndex);    
}

void TCPLinkUnitTest::_connectFail_test(void)
{
    Q_ASSERT(_link);
    Q_ASSERT(_multiSpy);
    Q_ASSERT(_multiSpy->checkNoSignals() == true);
    
    QCOMPARE(_link->connect(), false);

    // Make sure we get a linkError signal with the right link name
    QCOMPARE(_multiSpy->waitForSignalByIndex(communicationErrorSignalIndex, 1000), true);
    QCOMPARE(_multiSpy->checkOnlySignalByMask(communicationErrorSignalMask), true);
    QList<QVariant> arguments = _multiSpy->getSpyByIndex(communicationErrorSignalIndex)->takeFirst();
    QCOMPARE(arguments.at(0).toString(), _link->getName());
    _multiSpy->clearSignalByIndex(communicationErrorSignalIndex);

    // Try to connect again to make sure everything was cleaned up correctly from previous failed connection
    
    QCOMPARE(_link->connect(), false);
    
    // Make sure we get a linkError signal with the right link name
    QCOMPARE(_multiSpy->waitForSignalByIndex(communicationErrorSignalIndex, 1000), true);
    QCOMPARE(_multiSpy->checkOnlySignalByMask(communicationErrorSignalMask), true);
    arguments = _multiSpy->getSpyByIndex(communicationErrorSignalIndex)->takeFirst();
    QCOMPARE(arguments.at(0).toString(), _link->getName());
    _multiSpy->clearSignalByIndex(communicationErrorSignalIndex);
}

void TCPLinkUnitTest::_connectSucceed_test(void)
{
    Q_ASSERT(_link);
    Q_ASSERT(_multiSpy);
    Q_ASSERT(_multiSpy->checkNoSignals() == true);

    // Start the server side
    QTcpServer* server = new QTcpServer(this);
    QCOMPARE(server->listen(_hostAddress, _port), true);
    
    // Connect to the server
    QCOMPARE(_link->connect(), true);
    
    // Wait for the connection to come through on server side
    QCOMPARE(server->waitForNewConnection(1000), true);
    QTcpSocket* serverSocket = server->nextPendingConnection();
    Q_ASSERT(serverSocket);
    
    // Make sure we get the two different connected signals
    QCOMPARE(_multiSpy->waitForSignalByIndex(connectedSignalIndex, 1000), true);
    QCOMPARE(_multiSpy->checkOnlySignalByMask(connectedSignalMask | connected2SignalMask), true);
    QList<QVariant> arguments = _multiSpy->getSpyByIndex(connected2SignalIndex)->takeFirst();
    QCOMPARE(arguments.at(0).toBool(), true);
    _multiSpy->clearSignalsByMask(connectedSignalMask);
    
    // Test server->link data path

    QByteArray bytesOut("test");
    
    // Write data from server to link
    serverSocket->write(bytesOut);
    
    // Make sure we get the bytesReceived signal, with the correct data
    QCOMPARE(_multiSpy->waitForSignalByIndex(bytesReceivedSignalIndex, 1000), true);
    QCOMPARE(_multiSpy->checkOnlySignalByMask(bytesReceivedSignalMask), true);
    arguments = _multiSpy->getSpyByIndex(bytesReceivedSignalIndex)->takeFirst();
    QCOMPARE(arguments.at(1), QVariant(bytesOut));
    _multiSpy->clearSignalByIndex(bytesReceivedSignalIndex);
    
    // Test link->server data path
    
    // Write data from link to server
    _link->writeBytes(bytesOut.data(), bytesOut.size());
    QCOMPARE(_link->getSocket()->waitForBytesWritten(1000), true);

    // Make sure we get readyRead on server
    QCOMPARE(serverSocket->waitForReadyRead(1000), true);

    // Read the data and make sure it matches
    QByteArray bytesIn = serverSocket->read(bytesOut.size() + 100);
    QCOMPARE(bytesIn, bytesOut);
    
    // Disconnect the link
    _link->disconnect();
    
    // Make sure we get the disconnected signals on link side
    QCOMPARE(_multiSpy->waitForSignalByIndex(disconnectedSignalIndex, 1000), true);
    QCOMPARE(_multiSpy->checkOnlySignalByMask(disconnectedSignalMask | connected2SignalMask), true);
    arguments = _multiSpy->getSpyByIndex(connected2SignalIndex)->takeFirst();
    QCOMPARE(arguments.at(0).toBool(), false);
    _multiSpy->clearSignalsByMask(disconnectedSignalMask);
    
    // Make sure we get disconnected signals from the server side
    QCOMPARE(serverSocket->waitForDisconnected(1000), true  );
    
    // Try to connect again to make sure everything was cleaned up correctly from previous connection
    
    // Connect to the server
    QCOMPARE(_link->connect(), true);
    
    // Wait for the connection to come through on server side
    QCOMPARE(server->waitForNewConnection(1000), true);
    serverSocket = server->nextPendingConnection();
    Q_ASSERT(serverSocket);
    
    // Make sure we get the two different connected signals
    QCOMPARE(_multiSpy->waitForSignalByIndex(connectedSignalIndex, 1000), true);
    QCOMPARE(_multiSpy->checkOnlySignalByMask(connectedSignalMask | connected2SignalMask), true);
    arguments = _multiSpy->getSpyByIndex(connected2SignalIndex)->takeFirst();
    QCOMPARE(arguments.at(0).toBool(), true);
    _multiSpy->clearSignalsByMask(connectedSignalMask);

    delete server;
}