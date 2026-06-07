// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLCHANGESET_P_H
#define QQMLCHANGESET_P_H

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

#include <QtQmlIntegration/qqmlintegration.h>
#include <QtCore/qdebug.h>
#include <QtCore/qvector.h>
#include <QtQmlModels/private/qtqmlmodelsglobal_p.h>

QT_BEGIN_NAMESPACE

class Q_QMLMODELS_EXPORT QQmlChangeSet
{
    Q_GADGET
    QML_ANONYMOUS
public:
    struct MoveKey
    {
        MoveKey() {}
        MoveKey(int moveId, int offset) : moveId(moveId), offset(offset) {}
        int moveId = -1;
        int offset = 0;
    };

    // The storrage for Change (below). This struct is trivial, which it has to be in order to store
    // it in a QV4::Heap::Base object. The Change struct doesn't add any storage fields, so it is
    // safe to cast ChangeData to/from Change.
    struct ChangeData
    {
        int index;
        int count;
        int moveId;
        int offset;
    };

    struct Change: ChangeData
    {
        Change() {
            index = 0;
            count = 0;
            moveId = -1;
            offset = 0;
        }
        Change(int index, int count, int moveId = -1, int offset = 0) {
            this->index = index;
            this->count = count;
            this->moveId = moveId;
            this->offset = offset;
        }

        bool isMove() const { return moveId >= 0; }

        MoveKey moveKey(int index) const {
            return MoveKey(moveId, index - Change::index + offset); }

        int start() const { return index; }
        int end() const { return index + count; }
    };

    QQmlChangeSet();
    QQmlChangeSet(const QQmlChangeSet &changeSet);
    ~QQmlChangeSet();

    QQmlChangeSet &operator =(const QQmlChangeSet &changeSet);

    const QVector<Change> &removes() const { return m_removes; }
    const QVector<Change> &inserts() const { return m_inserts; }
    const QVector<Change> &changes() const { return m_changes; }

    void insert(int index, int count);
    void remove(int index, int count);
    void move(int from, int to, int count, int moveId);
    void change(int index, int count);

    void insert(const QVector<Change> &inserts);
    void remove(const QVector<Change> &removes, QVector<Change> *inserts = nullptr);
    void move(const QVector<Change> &removes, const QVector<Change> &inserts);
    void change(const QVector<Change> &changes);
    void apply(const QQmlChangeSet &changeSet);

    bool isEmpty() const { return m_removes.empty() && m_inserts.empty() && m_changes.isEmpty(); }

    void clear()
    {
        m_removes.clear();
        m_inserts.clear();
        m_changes.clear();
        m_difference = 0;
    }

    int difference() const { return m_difference; }

private:
    void remove(QVector<Change> *removes, QVector<Change> *inserts);
    void change(QVector<Change> *changes);

    QVector<Change> m_removes;
    QVector<Change> m_inserts;
    QVector<Change> m_changes;
    int m_difference;
};

Q_DECLARE_TYPEINFO(QQmlChangeSet::Change, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(QQmlChangeSet::MoveKey, Q_PRIMITIVE_TYPE);

inline size_t qHash(const QQmlChangeSet::MoveKey &key) { return qHash(std::make_pair(key.moveId, key.offset)); }
inline bool operator ==(const QQmlChangeSet::MoveKey &l, const QQmlChangeSet::MoveKey &r) {
    return l.moveId == r.moveId && l.offset == r.offset; }

Q_QMLMODELS_EXPORT QDebug operator <<(QDebug debug, const QQmlChangeSet::Change &change);
Q_QMLMODELS_EXPORT QDebug operator <<(QDebug debug, const QQmlChangeSet &change);

QT_END_NAMESPACE

#endif
