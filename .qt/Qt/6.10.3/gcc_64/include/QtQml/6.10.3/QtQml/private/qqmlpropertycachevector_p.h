// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLPROPERTYCACHEVECTOR_P_H
#define QQMLPROPERTYCACHEVECTOR_P_H

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

#include <private/qqmlpropertycache_p.h>
#include <private/qbipointer_p.h>

#include <QtCore/qtaggedpointer.h>

QT_BEGIN_NAMESPACE

class QQmlPropertyCacheVector
{
public:
    QQmlPropertyCacheVector() = default;
    QQmlPropertyCacheVector(QQmlPropertyCacheVector &&) = default;
    QQmlPropertyCacheVector &operator=(QQmlPropertyCacheVector &&) = default;

    ~QQmlPropertyCacheVector() { clear(); }
    void resize(int size)
    {
        Q_ASSERT(size >= data.size());
        return data.resize(size);
    }

    int count() const {
        // the property cache vector will never contain more thant INT_MAX many elements
        return int(data.size());
    }

    bool isEmpty() const { return data.isEmpty(); }

    void clear()
    {
        for (int i = 0; i < data.size(); ++i)
            releaseElement(i);
        data.clear();
    }

    void resetAndResize(int size)
    {
        for (int i = 0; i < data.size(); ++i) {
            releaseElement(i);
            data[i] = BiPointer();
        }
        data.resize(size);
    }

    void append(const QQmlPropertyCache::ConstPtr &cache) {
        cache->addref();
        data.append(BiPointer(cache.data()));
        Q_ASSERT(data.last().isT1());
        Q_ASSERT(data.size() <= std::numeric_limits<int>::max());
    }

    void appendOwn(const QQmlPropertyCache::Ptr &cache) {
        cache->addref();
        data.append(BiPointer(cache.data()));
        Q_ASSERT(data.last().isT2());
        Q_ASSERT(data.size() <= std::numeric_limits<int>::max());
    }

    QQmlPropertyCache::ConstPtr at(int index) const
    {
        const auto entry = data.at(index);
        if (entry.isT2())
            return entry.asT2();
        return entry.asT1();
    }

    QQmlPropertyCache::Ptr ownAt(int index) const
    {
        const auto entry = data.at(index);
        if (entry.isT2())
            return entry.asT2();
        return QQmlPropertyCache::Ptr();
    }

    void set(int index, const QQmlPropertyCache::ConstPtr &replacement) {
        if (QQmlPropertyCache::ConstPtr oldCache = at(index)) {
            // If it is our own, we keep it our own
            if (replacement.data() == oldCache.data())
                return;
            oldCache->release();
        }
        data[index] = replacement.data();
        if (replacement)
            replacement->addref();
        Q_ASSERT(data[index].isT1());
    }

    void setOwn(int index, const QQmlPropertyCache::Ptr &replacement) {
        if (QQmlPropertyCache::ConstPtr oldCache = at(index)) {
            if (replacement.data() != oldCache.data()) {
                oldCache->release();
                replacement->addref();
            }
        } else {
            replacement->addref();
        }
        data[index] = replacement.data();
        Q_ASSERT(data[index].isT2());
    }

    void setNeedsVMEMetaObject(int index) { data[index].setFlag(); }
    bool needsVMEMetaObject(int index) const { return data.at(index).flag(); }

    void seal()
    {
        for (auto &entry: data) {
            if (entry.isT2())
                entry = static_cast<const QQmlPropertyCache *>(entry.asT2());
            Q_ASSERT(entry.isT1());
        }
    }

private:
    void releaseElement(int i)
    {
        const auto &cache = data.at(i);
        if (cache.isT2()) {
            if (QQmlPropertyCache *data = cache.asT2())
                data->release();
        } else if (const QQmlPropertyCache *data = cache.asT1()) {
            data->release();
        }
    }

    Q_DISABLE_COPY(QQmlPropertyCacheVector)
    using BiPointer = QBiPointer<const QQmlPropertyCache, QQmlPropertyCache>;
    QVector<BiPointer> data;
};

QT_END_NAMESPACE

#endif // QQMLPROPERTYCACHEVECTOR_P_H
