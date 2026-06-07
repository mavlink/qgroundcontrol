// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef HPACK_P_H
#define HPACK_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "hpacktable_p.h"

#include <QtCore/qurl.h>

#include <vector>
#include <optional>

QT_BEGIN_NAMESPACE

class QByteArray;

namespace HPack
{

using HttpHeader = std::vector<HeaderField>;
HeaderSize header_size(const HttpHeader &header);
struct BitPattern;
class Q_AUTOTEST_EXPORT Encoder
{
public:
    Encoder(quint32 maxTableSize, bool compressStrings);

    quint32 dynamicTableSize() const;
    quint32 dynamicTableCapacity() const;
    quint32 maxDynamicTableCapacity() const;

    bool encodeRequest(class BitOStream &outputStream,
                       const HttpHeader &header);
    bool encodeResponse(BitOStream &outputStream,
                        const HttpHeader &header);

    bool encodeSizeUpdate(BitOStream &outputStream,
                          quint32 newSize);

    void setMaxDynamicTableSize(quint32 size);
    void setCompressStrings(bool compress);

private:
    bool encodeRequestPseudoHeaders(BitOStream &outputStream,
                                    const HttpHeader &header);
    bool encodeHeaderField(BitOStream &outputStream,
                           const HeaderField &field);
    bool encodeMethod(BitOStream &outputStream,
                      const HeaderField &field);

    bool encodeResponsePseudoHeaders(BitOStream &outputStream,
                                     const HttpHeader &header);

    bool encodeIndexedField(BitOStream &outputStream, quint32 index) const;


    bool encodeLiteralField(BitOStream &outputStream,
                            BitPattern fieldType,
                            quint32 nameIndex,
                            const QByteArray &value,
                            bool withCompression);

    bool encodeLiteralField(BitOStream &outputStream,
                            BitPattern fieldType,
                            const QByteArray &name,
                            const QByteArray &value,
                            bool withCompression);

    FieldLookupTable lookupTable;
    bool compressStrings;
};

class Q_AUTOTEST_EXPORT Decoder
{
public:
    Decoder(quint32 maxTableSize);

    bool decodeHeaderFields(class BitIStream &inputStream);

    const HttpHeader &decodedHeader() const
    {
        return header;
    }

    quint32 dynamicTableSize() const;
    quint32 dynamicTableCapacity() const;
    quint32 maxDynamicTableCapacity() const;

    void setMaxDynamicTableSize(quint32 size);

private:

    bool decodeIndexedField(BitIStream &inputStream);
    bool decodeSizeUpdate(BitIStream &inputStream);
    bool decodeLiteralField(BitPattern fieldType,
                            BitIStream &inputStream);

    bool processDecodedField(BitPattern fieldType,
                             const QByteArray &name,
                             const QByteArray &value);

    void handleStreamError(BitIStream &inputStream);

    HttpHeader header;
    FieldLookupTable lookupTable;
};

std::optional<QUrl> makePromiseKeyUrl(const HttpHeader &requestHeader);
}

QT_END_NAMESPACE

#endif

