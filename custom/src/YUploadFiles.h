/*!
 *   @brief Typhoon H QGCCorePlugin Declaration
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#pragma once

#include <QObject>
#include <QThread>
#include <QMetaType>
#include <QTcpSocket>

typedef  struct YuneecPacket {
    uint32_t 	type;
    uint32_t    size;
} YuneecPacket_t;


//-----------------------------------------------------------------------------
class YUploadFiles : public QObject
{
    Q_OBJECT
public:
    YUploadFiles    (QObject* parent);
    ~YUploadFiles   ();

    void        init                        (QHostAddress address, uint16_t port);

signals:

private slots:
    void        _connected                  ();
    void        _socketError                (QAbstractSocket::SocketError socketError);
    void        _readBytes                  ();

private:
    QTcpSocket*       _socket;

private:

};
