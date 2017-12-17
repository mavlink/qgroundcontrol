/*!
 *   @brief Typhoon H QGCCorePlugin Declaration
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#pragma once

#include <QObject>
#include <QThread>
#include <QMetaType>
#include <QTcpSocket>

//-----------------------------------------------------------------------------
class IRPC : public QObject
{
    Q_OBJECT
public:
    virtual ~IRPC() { }
    virtual bool    IsConnected         () = 0;
    virtual void    Close               () = 0;
    virtual bool    SendMessage         (uint32_t message, QByteArray data) = 0;
signals:
    void            connected           ();
    void            ConnectionClosed    ();
    void            HandleMessage       (uint32_t message, QByteArray data);
    void            ErrorMessage        (QString message);
};

//-----------------------------------------------------------------------------
class IClientRPC : public IRPC
{
public:
    virtual ~IClientRPC() { }
    virtual bool    Connect             (QHostAddress address, uint16_t port) = 0;
};

//-----------------------------------------------------------------------------
class IServerRPC : public IRPC
{
public:
    virtual ~IServerRPC() { }
    virtual bool    StartServer         (uint16_t port) = 0;
};

IClientRPC* getClientRPC();
IServerRPC* getServerRPC();
