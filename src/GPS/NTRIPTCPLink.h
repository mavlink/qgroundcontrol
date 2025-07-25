/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtCore/QUrl>

#include "rtcm.h"

Q_DECLARE_LOGGING_CATEGORY(NTRIPTCPLinkLog)

class NTRIPSettings;
class QTcpSocket;

class NTRIPTCPLink : public QThread
{
    Q_OBJECT

public:
    explicit NTRIPTCPLink(const QString &hostAddress,
                          int port,
                          const QString &username,
                          const QString &password,
                          const QString &mountpoint,
                          const QString &whitelist,
                          const bool &enableVRS,
                          QObject *parent = nullptr);
    ~NTRIPTCPLink();

signals:
    void error(const QString &errorMsg);
    void RTCMDataUpdate(const QByteArray &message);

protected:
    void run() final;

private slots:
    void _readBytes();

private:
    const QString _hostAddress;
    const int _port = 0;
    const QString _username;
    const QString _password;
    const QString _mountpoint;
    const bool _isVRSEnable = false;

    QSet<int> _whitelist;
    int _vrsSendRateMSecs = 3000;
    bool _ntripForceV1 = false;
    QUrl _ntripURL;

    QTimer *_vrsSendTimer = nullptr;
    QTcpSocket *_socket = nullptr;
    RTCMParsing *_rtcm_parsing = nullptr;

    enum class NTRIPState {
        uninitialised,
        waiting_for_http_response,
        waiting_for_rtcm_header,
        accumulating_rtcm_packet,
    };
    NTRIPState _state = NTRIPState::uninitialised;

    void _hardwareConnect();
    void _parse(const QByteArray &buffer);
    void _sendNmeaGga();
};
