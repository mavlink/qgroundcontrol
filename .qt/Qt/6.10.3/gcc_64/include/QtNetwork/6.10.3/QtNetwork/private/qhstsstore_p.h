// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QHSTSSTORE_P_H
#define QHSTSSTORE_P_H

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

#include <QtNetwork/private/qtnetworkglobal_p.h>

QT_REQUIRE_CONFIG(settings);

#include <QtCore/qlist.h>
#include <QtCore/qsettings.h>

QT_BEGIN_NAMESPACE

class QHstsPolicy;
class QByteArray;
class QString;

class Q_AUTOTEST_EXPORT QHstsStore
{
public:
    explicit QHstsStore(const QString &dirName);
    ~QHstsStore();

    QList<QHstsPolicy> readPolicies();
    void addToObserved(const QHstsPolicy &policy);
    void synchronize();

    bool isWritable() const;

    static QString absoluteFilePath(const QString &dirName);
private:
    void beginHstsGroups();
    bool serializePolicy(const QString &key, const QHstsPolicy &policy);
    bool deserializePolicy(const QString &key, QHstsPolicy &policy);
    void evictPolicy(const QString &key);
    void endHstsGroups();

    QList<QHstsPolicy> observedPolicies;
    QSettings store;

    Q_DISABLE_COPY_MOVE(QHstsStore)
};

QT_END_NAMESPACE

#endif // QHSTSSTORE_P_H
