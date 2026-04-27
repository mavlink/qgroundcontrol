/****************************************************************************
 *
 * CrownEagle video status receiver.
 *
 ****************************************************************************/

#include "CEVideoStatus.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtNetwork/QNetworkDatagram>

CEVideoStatus::CEVideoStatus(QObject *parent)
    : QObject(parent)
{
    _lastPacketTimer.start();

    (void) connect(&_socket, &QUdpSocket::readyRead, this, &CEVideoStatus::_readPendingDatagrams);
    _socket.bind(QHostAddress::AnyIPv4, 5601, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);

    _timeoutTimer.setInterval(1000);
    _timeoutTimer.setSingleShot(false);
    (void) connect(&_timeoutTimer, &QTimer::timeout, this, &CEVideoStatus::_checkTimeout);
    _timeoutTimer.start();
}

void CEVideoStatus::_readPendingDatagrams()
{
    while (_socket.hasPendingDatagrams()) {
        const QByteArray datagram = _socket.receiveDatagram().data();
        const QJsonDocument doc = QJsonDocument::fromJson(datagram);
        if (!doc.isObject()) {
            continue;
        }

        const QJsonObject obj = doc.object();
        const QString state = obj.value(QStringLiteral("state")).toString(QStringLiteral("NO_VIDEO"));
        const double fps = obj.value(QStringLiteral("fps")).toDouble(0.0);
        const double age = obj.value(QStringLiteral("last_frame_age_sec")).isNull()
            ? -1.0
            : obj.value(QStringLiteral("last_frame_age_sec")).toDouble(-1.0);

        _lastPacketTimer.restart();
        _setStatus(state, fps, age, true);
    }
}

void CEVideoStatus::_checkTimeout()
{
    if (_hasStatus && _lastPacketTimer.elapsed() > 3000) {
        _setStatus(QStringLiteral("NO_VIDEO"), 0.0, -1.0, false);
    }
}

void CEVideoStatus::_setStatus(const QString &state, double fps, double lastFrameAgeSec, bool hasStatus)
{
    if ((_state == state) && (_fps == fps) && (_lastFrameAgeSec == lastFrameAgeSec) && (_hasStatus == hasStatus)) {
        return;
    }

    _state = state;
    _fps = fps;
    _lastFrameAgeSec = lastFrameAgeSec;
    _hasStatus = hasStatus;
    emit statusChanged();
}
