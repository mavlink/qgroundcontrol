// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef HUFFMAN_P_H
#define HUFFMAN_P_H

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

QT_BEGIN_NAMESPACE

class QByteArray;

namespace HPack
{

struct CodeEntry
{
    quint32 byteValue;
    quint32 huffmanCode;
    quint32 bitLength;
};

class BitOStream;

quint64 huffman_encoded_bit_length(QByteArrayView inputData);
void huffman_encode_string(QByteArrayView inputData, BitOStream &outputStream);

// PrefixTable:
// Huffman codes with a small bit length
// fit into a table (these are 'terminal' symbols),
// codes with longer codes require additional
// tables, so several symbols will have the same index
// in a table - pointing into the next table.
// Every table has an 'indexLength' - that's
// how many bits can fit in table's indices +
// 'prefixLength' - how many bits were addressed
// by its 'parent' table(s).
// All PrefixTables are kept in 'prefixTables' array.
// PrefixTable itself does not have any entries,
// it just holds table's prefix/index + 'offset' -
// there table's data starts in an array of all
// possible entries ('tableData').

struct PrefixTable
{
    PrefixTable()
        : prefixLength(),
          indexLength(),
          offset()
    {
    }

    PrefixTable(quint32 prefix, quint32 index)
        : prefixLength(prefix),
          indexLength(index),
          offset()
    {
    }

    quint32 size()const
    {
        // Number of entries table contains:
        return 1 << indexLength;
    }

    quint32 prefixLength;
    quint32 indexLength;
    quint32 offset;
};

// Table entry is either a terminal entry (thus probably the code found)
// or points into another table ('nextTable' - index into
// 'prefixTables' array). If it's a terminal, 'nextTable' index
// refers to the same table.

struct PrefixTableEntry
{
    PrefixTableEntry()
        : bitLength(),
          nextTable(),
          byteValue()
    {
    }

    quint32 bitLength;
    quint32 nextTable;
    quint32 byteValue;
};

class BitIStream;

class HuffmanDecoder
{
public:
    enum class BitConstants
    {
        rootPrefix = 9,
        childPrefix = 6
    };

    HuffmanDecoder();

    bool decodeStream(BitIStream &inputStream, QByteArray &outputBuffer);

private:
    quint32 addTable(quint32 prefixLength, quint32 indexLength);
    PrefixTableEntry tableEntry(PrefixTable table, quint32 index);
    void setTableEntry(PrefixTable table, quint32 index, PrefixTableEntry entry);

    std::vector<PrefixTable> prefixTables;
    std::vector<PrefixTableEntry> tableData;
    quint32 minCodeLength;
};

bool huffman_decode_string(BitIStream &inputStream, QByteArray *outputBuffer);

} // namespace HPack

QT_END_NAMESPACE

#endif

