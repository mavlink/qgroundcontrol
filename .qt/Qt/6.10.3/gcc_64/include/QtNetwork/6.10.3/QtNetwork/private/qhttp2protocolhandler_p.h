// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QHTTP2PROTOCOLHANDLER_P_H
#define QHTTP2PROTOCOLHANDLER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the Network Access API.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include "access/qhttp2connection_p.h"
#include <private/qhttpnetworkconnectionchannel_p.h>
#include <private/qabstractprotocolhandler_p.h>
#include <private/qhttpnetworkrequest_p.h>

#include <access/qhttp2configuration.h>

#include <private/qhttp2connection_p.h>
#include <private/http2protocol_p.h>
#include <private/http2streams_p.h>
#include <private/http2frames_p.h>
#include <private/hpacktable_p.h>
#include <private/hpack_p.h>

#include <QtCore/qnamespace.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qobject.h>
#include <QtCore/qflags.h>
#include <QtCore/qhash.h>

#include <vector>
#include <limits>
#include <memory>
#include <deque>
#include <set>

QT_REQUIRE_CONFIG(http);

QT_BEGIN_NAMESPACE

class QHttp2ProtocolHandler : public QObject, public QAbstractProtocolHandler
{
    Q_OBJECT

public:
    QHttp2ProtocolHandler(QHttpNetworkConnectionChannel *channel);

    QHttp2ProtocolHandler(const QHttp2ProtocolHandler &rhs) = delete;
    QHttp2ProtocolHandler(QHttp2ProtocolHandler &&rhs) = delete;

    QHttp2ProtocolHandler &operator=(const QHttp2ProtocolHandler &rhs) = delete;
    QHttp2ProtocolHandler &operator=(QHttp2ProtocolHandler &&rhs) = delete;

    Q_INVOKABLE void handleConnectionClosure();

private slots:
    void _q_uploadDataDestroyed(QObject *uploadData);

private:
    using Stream = Http2::Stream;

    void _q_readyRead() override;
    Q_INVOKABLE void _q_receiveReply() override;
    Q_INVOKABLE bool sendRequest() override;
    bool tryRemoveReply(QHttpNetworkReply *reply) override;

    bool sendSETTINGS_ACK();
    bool sendHEADERS(QHttp2Stream *stream, QHttpNetworkRequest &request);
    bool sendDATA(QHttp2Stream *stream, QHttpNetworkReply *reply);

    bool acceptSetting(Http2::Settings identifier, quint32 newValue);

    void handleAuthorization(QHttp2Stream *stream);
    void handleHeadersReceived(const HPack::HttpHeader &headers, bool endStream);
    void handleDataReceived(const QByteArray &data, bool endStream);
    void finishStream(QHttp2Stream *stream,
                      Qt::ConnectionType connectionType = Qt::DirectConnection);
    // Error code send by a peer (GOAWAY/RST_STREAM):
    void handleGOAWAY(Http2::Http2Error errorCode, quint32 lastStreamID);
    void finishStreamWithError(QHttp2Stream *stream, Http2::Http2Error errorCode);
    // Locally encountered error:
    void finishStreamWithError(QHttp2Stream *stream, QNetworkReply::NetworkError error,
                               const QString &message);

    // Stream's lifecycle management:
    QHttp2Stream *createNewStream(const HttpMessagePair &message, bool uploadDone = false);
    void connectStream(const HttpMessagePair &message, QHttp2Stream *stream);
    quint32 popStreamToResume();

    QHttp2Connection *h2Connection;

    // In the current implementation we send
    // SETTINGS only once, immediately after
    // the client's preface 24-byte message.
    bool waitingForSettingsACK = false;

    inline static const quint32 maxAcceptableTableSize = 16 * HPack::FieldLookupTable::DefaultSize;

    QHash<QObject *, QPointer<QHttp2Stream>> streamIDs;
    using HttpMessagePair = std::pair<QHttpNetworkRequest, QHttpNetworkReply *>;
    QHash<QHttp2Stream *, HttpMessagePair> requestReplyPairs;

    void initReplyFromPushPromise(const HttpMessagePair &message, const QUrl &cacheKey);
    // Errors:
    void connectionError(Http2::Http2Error errorCode, const QString &message);
    void closeSession();
};

QT_END_NAMESPACE

#endif
