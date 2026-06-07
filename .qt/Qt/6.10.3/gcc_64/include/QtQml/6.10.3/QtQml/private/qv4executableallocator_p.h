// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QV4EXECUTABLEALLOCATOR_H
#define QV4EXECUTABLEALLOCATOR_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QMultiMap>
#include <QHash>
#include <QVector>
#include <QByteArray>
#include <QMutex>

#include <QtQml/private/qtqmlglobal_p.h>

namespace WTF {
class PageAllocation;
}

QT_BEGIN_NAMESPACE

namespace QV4 {

class Q_QML_AUTOTEST_EXPORT ExecutableAllocator
{
public:
    struct ChunkOfPages;
    struct Allocation;

    ExecutableAllocator();
    ~ExecutableAllocator();

    Allocation *allocate(size_t size);
    void free(Allocation *allocation);

    struct Allocation
    {
        Allocation()
            : size(0)
            , free(true)
        {}

        void *memoryStart() const;
        size_t memorySize() const { return size; }

        void *exceptionHandlerStart() const;
        size_t exceptionHandlerSize() const;

        void *codeStart() const;

        void invalidate() { addr = 0; }
        bool isValid() const { return addr != 0; }
        void deallocate(ExecutableAllocator *allocator);

    private:
        ~Allocation() {}

        friend class ExecutableAllocator;

        Allocation *split(size_t dividingSize);
        bool mergeNext(ExecutableAllocator *allocator);
        bool mergePrevious(ExecutableAllocator *allocator);

        quintptr addr = 0;
        uint size : 31; // More than 2GB of function code? nah :)
        uint free : 1;
        Allocation *next = nullptr;
        Allocation *prev = nullptr;
    };

    // for debugging / unit-testing
    int freeAllocationCount() const { return freeAllocations.size(); }
    int chunkCount() const { return chunks.size(); }

    struct ChunkOfPages
    {
        ChunkOfPages()

        {}
        ~ChunkOfPages();

        WTF::PageAllocation *pages = nullptr;
        Allocation *firstAllocation = nullptr;

        bool contains(Allocation *alloc) const;
    };

    ChunkOfPages *chunkForAllocation(Allocation *allocation) const;

private:
    QMultiMap<size_t, Allocation*> freeAllocations;
    QMap<quintptr, ChunkOfPages*> chunks;
    mutable QMutex mutex;
};

}

QT_END_NAMESPACE

#endif // QV4EXECUTABLEALLOCATOR_H
