// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef HTTP2STREAMS_P_H
#define HTTP2STREAMS_P_H

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

#include "http2frames_p.h"
#include "hpack_p.h"

#include <private/qhttpnetworkconnectionchannel_p.h>
#include <private/qhttpnetworkrequest_p.h>

#include <QtCore/qstring.h>

#include <vector>

QT_REQUIRE_CONFIG(http);

QT_BEGIN_NAMESPACE

class QNonContiguousByteDevice;

namespace Http2
{

struct Q_AUTOTEST_EXPORT Stream
{
    enum StreamState {
        idle,
        open,
        halfClosedLocal,
        halfClosedRemote,
        remoteReserved,
        closed
    };

    Stream();
    // That's a ctor for a client-initiated stream:
    Stream(const HttpMessagePair &message, quint32 streamID, qint32 sendSize,
           qint32 recvSize);
    // That's a reserved stream, created by PUSH_PROMISE from a server:
    Stream(const QString &key, quint32 streamID, qint32 recvSize);

    QHttpNetworkReply *reply() const;
    const QHttpNetworkRequest &request() const;
    QHttpNetworkRequest &request();
    QHttpNetworkRequest::Priority priority() const;
    uchar weight() const;

    QNonContiguousByteDevice *data() const;

    HttpMessagePair httpPair;
    quint32 streamID = 0;
    // Signed as window sizes can become negative:
    qint32 sendWindow = 65535;
    qint32 recvWindow = 65535;

    StreamState state = idle;
    QString key; // for PUSH_PROMISE
};

struct PushPromise
{
    quint32 reservedID = 0;
    // PUSH_PROMISE has its own HEADERS,
    // usually similar to what request has:
    HPack::HttpHeader pushHeader;
    // Response has its own (normal) HEADERS:
    HPack::HttpHeader responseHeader;
    // DATA frames on a promised stream:
    std::vector<Frame> dataFrames;
};

} // namespace Http2

QT_END_NAMESPACE

#endif

