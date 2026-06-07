// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLROLESORTER_P_H
#define QQMLROLESORTER_P_H

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

#include <QtQmlModels/private/qqmlsorterbase_p.h>

QT_BEGIN_NAMESPACE

class QQmlSortFilterProxyModel;
class QQmlRoleSorterPrivate;

class Q_QMLMODELS_EXPORT QQmlRoleSorter : public QQmlSorterBase
{
    Q_OBJECT
    Q_PROPERTY(QString roleName READ roleName WRITE setRoleName NOTIFY roleNameChanged)
    QML_NAMED_ELEMENT(RoleSorter)
    QML_ADDED_IN_VERSION(6, 10)

public:
    explicit QQmlRoleSorter(QObject *parent = nullptr);
    QQmlRoleSorter(QQmlSorterBasePrivate *priv, QObject *parent = nullptr);
    ~QQmlRoleSorter() = default;

    const QString& roleName() const;
    void setRoleName(const QString& roleName);

    QPartialOrdering compare(const QModelIndex& sourceLeft, const QModelIndex& sourceRight, const QQmlSortFilterProxyModel *proxyModel) const override;

Q_SIGNALS:
    void roleNameChanged();

private:
    Q_DECLARE_PRIVATE(QQmlRoleSorter)
};

class QQmlRoleSorterPrivate : public QQmlSorterBasePrivate
{
    Q_DECLARE_PUBLIC (QQmlRoleSorter)

public:
    QString m_roleName = QString::fromUtf8("display");
};

QT_END_NAMESPACE

#endif // QQMLROLESORTER_P_H
