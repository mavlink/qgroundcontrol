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

#ifndef TCPLOOPBACKSERVER_H
#define TCPLOOPBACKSERVER_H

#include <QThread>
#include <QTcpServer>
#include <QTcpSocket>

/// @file
///     @brief Simple TCP loop back server
///
///     @author Don Gagne <don@thegagnes.com>

class TCPLoopBackServer : public QThread
{
    Q_OBJECT
    
public:
    TCPLoopBackServer(QHostAddress hostAddress, quint16 port);
    
signals:
    void newConnection(void);
    
protected:
    virtual void run(void);
    
private slots:
    void _newConnection(void);
    void _readBytes(void);
    
private:
    QHostAddress    _hostAddress;
    quint16         _port;
    QTcpServer*     _tcpServer;
    QTcpSocket*     _tcpSocket;
};

#endif