// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLPROPERTYRESOLVER_P_H
#define QQMLPROPERTYRESOLVER_P_H

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

#include <private/qtqmlglobal_p.h>
#include <private/qqmlpropertycache_p.h>
#include <private/qqmlrefcount_p.h>

QT_BEGIN_NAMESPACE

struct Q_QML_EXPORT QQmlPropertyResolver
{
    QQmlPropertyResolver(const QQmlPropertyCache::ConstPtr &cache)
        : cache(cache)
    {}

    const QQmlPropertyData *property(int index) const
    {
        return cache->property(index);
    }

    enum RevisionCheck {
        CheckRevision,
        IgnoreRevision
    };

    const QQmlPropertyData *property(
            const QString &name, bool *notInRevision = nullptr,
            RevisionCheck check = CheckRevision) const;

    // This code must match the semantics of QQmlPropertyPrivate::findSignalByName
    const QQmlPropertyData *signal(
            const QString &name, bool *notInRevision = nullptr,
            RevisionCheck check = CheckRevision) const;

    QQmlPropertyCache::ConstPtr cache;
};

QT_END_NAMESPACE

#endif // QQMLPROPERTYRESOLVER_P_H
