/*!
 *   @brief Typhoon H QGCCorePlugin Implementation
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#include "YUploadFiles.h"
#include "QGCApplication.h"
#include "AppSettings.h"
#include "SettingsManager.h"
#include "UTMConverter.h"
#include <QDirIterator>

//-----------------------------------------------------------------------------
YUploadFiles::YUploadFiles(QObject* parent)
    : QObject(parent)
    , _socket(NULL)
{

}

//-----------------------------------------------------------------------------
YUploadFiles::~YUploadFiles()
{
    if(_socket) {
        _socket->deleteLater();
    }
}

//-----------------------------------------------------------------------------
void
YUploadFiles::init(QHostAddress address, uint16_t port)
{
    if(!_socket) {
        _socket = new QTcpSocket();
        connect(_socket, &QTcpSocket::readyRead, this, &YUploadFiles::_readBytes);
        connect(_socket, &QTcpSocket::connected, this, &YUploadFiles::_connected);
        connect(_socket, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error), this, &YUploadFiles::_socketError);
    }
}

//-----------------------------------------------------------------------------
void
YUploadFiles::cancel()
{

}

