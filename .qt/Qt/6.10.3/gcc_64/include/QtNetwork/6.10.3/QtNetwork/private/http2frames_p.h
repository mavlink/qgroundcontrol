// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:network-protocol

#ifndef HTTP2FRAMES_P_H
#define HTTP2FRAMES_P_H

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

#include "http2protocol_p.h"
#include "hpack_p.h"

#include <QtCore/qendian.h>
#include <algorithm>
#include <vector>

QT_BEGIN_NAMESPACE

class QHttp2ProtocolHandler;
class QIODevice;

namespace Http2
{

struct Q_AUTOTEST_EXPORT Frame
{
    Frame();
    // Reading these values without first forming a valid frame (either reading
    // it from a socket or building it) will result in undefined behavior:
    FrameType type() const;
    quint32 streamID() const;
    FrameFlags flags() const;
    quint32 payloadSize() const;
    uchar padding() const;
    // In HTTP/2 a stream's priority is specified by its weight and a stream
    // (id) it depends on:
    bool priority(quint32 *streamID = nullptr,
                  uchar *weight = nullptr) const;

    FrameStatus validateHeader() const;
    FrameStatus validatePayload() const;

    // Number of payload bytes without padding and/or priority.
    quint32 dataSize() const;
    // HEADERS data size for HEADERS, PUSH_PROMISE and CONTINUATION streams:
    quint32 hpackBlockSize() const;
    // Beginning of payload without priority/padding bytes.
    const uchar *dataBegin() const;
    // HEADERS data beginning for HEADERS, PUSH_PROMISE and CONTINUATION streams:
    const uchar *hpackBlockBegin() const;

    std::vector<uchar> buffer;
};

class Q_AUTOTEST_EXPORT FrameReader
{
public:
    FrameStatus read(QIODevice &socket);

    Frame &inboundFrame()
    {
        return frame;
    }
private:
    bool readHeader(QIODevice &socket);
    bool readPayload(QIODevice &socket);
    bool discardPayload(QIODevice &socket);

    quint32 offset = 0;
    Frame frame;
};

class Q_AUTOTEST_EXPORT FrameWriter
{
public:
    using payload_type = std::vector<uchar>;
    using size_type = payload_type::size_type;

    FrameWriter();
    FrameWriter(FrameType type, FrameFlags flags, quint32 streamID);

    Frame &outboundFrame()
    {
        return frame;
    }

    void setOutboundFrame(Frame &&newFrame);

    // Frame 'builders':
    void start(FrameType type, FrameFlags flags, quint32 streamID);
    void setPayloadSize(quint32 size);
    void setType(FrameType type);
    void setFlags(FrameFlags flags);
    void addFlag(FrameFlag flag);

    // All append functions also update frame's payload length.
    template<typename ValueType>
    void append(ValueType val)
    {
        uchar wired[sizeof val] = {};
        qToBigEndian(val, wired);
        append(wired, wired + sizeof val);
    }
    void append(uchar val)
    {
        frame.buffer.push_back(val);
        updatePayloadSize();
    }
    void append(Settings identifier)
    {
        append(quint16(identifier));
    }
    void append(const payload_type &payload)
    {
        append(&payload[0], &payload[0] + payload.size());
    }
    void append(QByteArrayView payload)
    {
        append(reinterpret_cast<const uchar *>(payload.begin()),
               reinterpret_cast<const uchar *>(payload.end()));
    }

    void append(const uchar *begin, const uchar *end);

    // Write as a single frame:
    bool write(QIODevice &socket) const;
    // Two types of frames we are sending are affected by frame size limits:
    // HEADERS and DATA. HEADERS' payload (hpacked HTTP headers, following a
    // frame header) is always in our 'buffer', we send the initial HEADERS
    // frame first and then CONTINUTATION frame(s) if needed:
    bool writeHEADERS(QIODevice &socket, quint32 sizeLimit);
    // With DATA frames the actual payload is never in our 'buffer', it's a
    // 'readPointer' from QNonContiguousData. We split this payload as needed
    // into DATA frames with correct payload size fitting into frame size limit:
    bool writeDATA(QIODevice &socket, quint32 sizeLimit,
                   const uchar *src, quint32 size);
private:
    void updatePayloadSize();
    Frame frame;
};

}

QT_END_NAMESPACE

#endif
