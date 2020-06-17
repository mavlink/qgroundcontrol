/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#pragma once

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

