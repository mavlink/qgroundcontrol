// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLROLEFILTER_P_H
#define QQMLROLEFILTER_P_H

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

#include <QtQmlModels/private/qqmlfilterbase_p.h>

QT_BEGIN_NAMESPACE

class QQmlSortFilterProxyModel;
class QQmlRoleFilterPrivate;

class Q_QMLMODELS_EXPORT QQmlRoleFilter : public QQmlFilterBase
{
    Q_OBJECT
    Q_PROPERTY(QString roleName READ roleName WRITE setRoleName NOTIFY roleNameChanged)
    QML_NAMED_ELEMENT(RoleFilter)
    QML_UNCREATABLE("")
    QML_ADDED_IN_VERSION(6, 10)

public:
    explicit QQmlRoleFilter(QObject *parent = nullptr);
    QQmlRoleFilter(QQmlFilterBasePrivate *priv, QObject *parent = nullptr);
    ~QQmlRoleFilter() = default;

    const QString& roleName() const;
    void setRoleName(const QString& roleName);

Q_SIGNALS:
    void roleNameChanged();

protected:
    int itemRole(const QQmlSortFilterProxyModel *proxyModel) const;

private:
    Q_DECLARE_PRIVATE(QQmlRoleFilter)
};

class QQmlRoleFilterPrivate : public QQmlFilterBasePrivate
{
    Q_DECLARE_PUBLIC (QQmlRoleFilter)

public:
    QString m_roleName = QString::fromUtf8("display");
};

QT_END_NAMESPACE

#endif // QQMLROLEFILTER_P_H
