// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef HPACKTABLE_P_H
#define HPACKTABLE_P_H

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

#include <QtCore/qbytearray.h>

#include <vector>
#include <memory>
#include <deque>
#include <set>
#include <utility>

QT_BEGIN_NAMESPACE

namespace HPack
{

struct Q_AUTOTEST_EXPORT HeaderField
{
    HeaderField()
    {
    }

    HeaderField(const QByteArray &n, const QByteArray &v)
        : name(n),
          value(v)
    {
    }

    bool operator == (const HeaderField &rhs) const
    {
        return name == rhs.name && value == rhs.value;
    }

    QByteArray name;
    QByteArray value;
};

using HeaderSize = std::pair<bool, quint32>;

HeaderSize entry_size(QByteArrayView name, QByteArrayView value);

inline HeaderSize entry_size(const HeaderField &entry)
{
    return entry_size(entry.name, entry.value);
}

/*
    Lookup table consists of two parts (HPACK, 2.3):
    the immutable static table (pre-defined by HPACK's specs)
    and dynamic table which is updated while
    compressing/decompressing headers.

    Table must provide/implement:
    1. Fast random access - we read fields' indices from
       HPACK's bit stream.
    2. FIFO for dynamic part - to push new items to the front
       and evict them from the back (HPACK, 2.3.2).
    3. Fast lookup - encoder receives pairs of strings
       (name|value) and it has to find an index for a pair
       as the whole or  for a name at least (if it's already
       in either static or dynamic table).

    Static table is an immutable vector.

    Dynamic part is implemented in a way similar to std::deque -
    it's a vector of pointers to chunks. Each chunk is a vector of
    (name|value) pairs. Once allocated with a fixed size, chunk
    never re-allocates its data, so entries' addresses do not change.
    We add new chunks prepending them to the front of a vector,
    in each chunk we fill (name|value) pairs starting from the back
    of the chunk (this simplifies item eviction/FIFO).
    Given a 'linear' index we can find a chunk number and
    offset in this chunk - random access.

    Lookup in a static part is straightforward:
    it's an (immutable) vector, data is sorted,
    contains no duplicates, we use binary search comparing string values.

    To provide a lookup in dynamic table faster than a linear search,
    we have an std::set of 'SearchEntries', where each entry contains:
     - a pointer to a (name|value) pair (to compare
       name|value strings).
     - a pointer to a chunk containing this pair and
     - an offset within this chunk - to calculate a
       'linear' index.

    Entries in a table can be duplicated (HPACK, 2.3.2),
    if we evict an entry, we must update our index removing
    the exactly right key, thus keys in this set are sorted
    by name|value pairs first, and then by chunk index/offset
    (so that NewSearchEntryKey < OldSearchEntry even if strings
    are equal).
*/

class Q_AUTOTEST_EXPORT FieldLookupTable
{
public:
    enum
    {
        ChunkSize = 16,
        DefaultSize = 4096 // Recommended by HTTP2.
    };

    FieldLookupTable(quint32 maxTableSize, bool useIndex);

    bool prependField(const QByteArray &name, const QByteArray &value);
    void evictEntry();

    quint32 numberOfEntries() const;
    quint32 numberOfStaticEntries() const;
    quint32 numberOfDynamicEntries() const;
    quint32 dynamicDataSize() const;
    quint32 dynamicDataCapacity() const;
    quint32 maxDynamicDataCapacity() const;
    void clearDynamicTable();

    bool indexIsValid(quint32 index) const;
    quint32 indexOf(const QByteArray &name, const QByteArray &value) const;
    quint32 indexOf(const QByteArray &name) const;
    bool field(quint32 index, QByteArray *name, QByteArray *value) const;
    bool fieldName(quint32 index, QByteArray *dst) const;
    bool fieldValue(quint32 index, QByteArray *dst) const;

    bool updateDynamicTableSize(quint32 size);
    void setMaxDynamicTableSize(quint32 size);

    static const std::vector<HeaderField> &staticPart();

private:
    // Table's maximum size is controlled
    // by SETTINGS_HEADER_TABLE_SIZE (HTTP/2, 6.5.2).
    quint32 maxTableSize;
    // The tableCapacity is how many bytes the table
    // can currently hold. It cannot exceed maxTableSize.
    // It can be modified by a special message in
    // the HPACK bit stream (HPACK, 6.3).
    quint32 tableCapacity;

    using Chunk = std::vector<HeaderField>;
    using ChunkPtr = std::unique_ptr<Chunk>;
    std::deque<ChunkPtr> chunks;
    using size_type = std::deque<ChunkPtr>::size_type;

    struct SearchEntry;
    friend struct SearchEntry;

    struct SearchEntry
    {
        SearchEntry();
        SearchEntry(const HeaderField *f, const Chunk *c,
                    quint32 o, const FieldLookupTable *t);

        const HeaderField *field;
        const Chunk *chunk;
        const quint32 offset;
        const FieldLookupTable *table;

        bool operator < (const SearchEntry &rhs) const;
    };

    bool useIndex;
    std::set<SearchEntry> searchIndex;

    SearchEntry frontKey() const;
    SearchEntry backKey() const;

    bool fieldAt(quint32 index, HeaderField *field) const;

    const HeaderField &front() const;
    HeaderField &front();
    const HeaderField &back() const;

    quint32 nDynamic;
    quint32 begin;
    quint32 end;
    quint32 dataSize;

    quint32 indexOfChunk(const Chunk *chunk) const;
    quint32 keyToIndex(const SearchEntry &key) const;

    enum class CompareMode {
        nameOnly,
        nameAndValue
    };

    static std::vector<HeaderField>::const_iterator findInStaticPart(const HeaderField &field, CompareMode mode);

    mutable QByteArray dummyDst;

    Q_DISABLE_COPY_MOVE(FieldLookupTable)
};

}

QT_END_NAMESPACE

#endif
