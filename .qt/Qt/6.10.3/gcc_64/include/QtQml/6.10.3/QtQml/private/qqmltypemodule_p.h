// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLTYPEMODULE_P_H
#define QQMLTYPEMODULE_P_H

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

#include <QtQml/qtqmlglobal.h>
#include <QtQml/private/qstringhash_p.h>
#include <QtQml/private/qqmltype_p.h>
#include <QtCore/qmutex.h>
#include <QtCore/qstring.h>
#include <QtCore/qversionnumber.h>

#include <functional>

QT_BEGIN_NAMESPACE

class QQmlType;
class QQmlTypePrivate;
struct QQmlMetaTypeData;

namespace QV4 {
struct String;
}

class QQmlTypeModule
{
public:
    enum class LockLevel {
        Open = 0,
        Weak = 1,
        Strong = 2
    };

    QQmlTypeModule() = default;
    QQmlTypeModule(const QString &uri, quint8 majorVersion)
        : m_module(uri), m_majorVersion(majorVersion)
    {}

    void add(QQmlTypePrivate *);
    void remove(const QQmlTypePrivate *type);

    LockLevel lockLevel() const { return LockLevel(m_lockLevel.loadRelaxed()); }
    bool setLockLevel(LockLevel mode)
    {
        while (true) {
            const int currentLock = m_lockLevel.loadAcquire();
            if (currentLock > int(mode))
                return false;
            if (currentLock == int(mode) || m_lockLevel.testAndSetRelease(currentLock, int(mode)))
                return true;
        }
    }

    QString module() const
    {
        // No need to lock. m_module is const
        return m_module;
    }

    quint8 majorVersion() const
    {
        // No need to lock. d->majorVersion is const
        return m_majorVersion;
    }

    void addMinorVersion(quint8 minorVersion);
    quint8 minimumMinorVersion() const { return m_minMinorVersion.loadRelaxed(); }
    quint8 maximumMinorVersion() const { return m_maxMinorVersion.loadRelaxed(); }

    QQmlType type(const QHashedStringRef &name, QTypeRevision version) const
    {
        QMutexLocker lock(&m_mutex);
        return findType(m_typeHash.value(name), version);
    }

    QQmlType type(const QV4::String *name, QTypeRevision version) const
    {
        QMutexLocker lock(&m_mutex);
        return findType(m_typeHash.value(name), version);
    }

    void walkCompositeSingletons(const std::function<void(const QQmlType &)> &callback) const;

private:
    static Q_QML_EXPORT QQmlType findType(
            const QList<QQmlTypePrivate *> *types, QTypeRevision version);

    const QString m_module;
    const quint8 m_majorVersion = 0;

    // Can only ever decrease
    QAtomicInt m_minMinorVersion = std::numeric_limits<quint8>::max();

    // Can only ever increase
    QAtomicInt m_maxMinorVersion = 0;

    // LockLevel. Can only be increased.
    QAtomicInt m_lockLevel = int(LockLevel::Open);

    using TypeHash = QStringHash<QList<QQmlTypePrivate *>>;
    TypeHash m_typeHash;

    mutable QMutex m_mutex;
};

QT_END_NAMESPACE

#endif // QQMLTYPEMODULE_P_H
