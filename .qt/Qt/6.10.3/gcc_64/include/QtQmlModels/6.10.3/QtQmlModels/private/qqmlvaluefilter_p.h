// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLVALUEFILTER_P_H
#define QQMLVALUEFILTER_P_H

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

#include <QtQmlModels/private/qqmlrolefilter_p.h>

QT_BEGIN_NAMESPACE

class QQmlSortFilterProxyModel;
class QQmlValueFilterPrivate;

class Q_QMLMODELS_EXPORT QQmlValueFilter : public QQmlRoleFilter
{
    Q_OBJECT
    Q_PROPERTY(QVariant value READ value WRITE setValue RESET resetValue NOTIFY valueChanged)
    QML_NAMED_ELEMENT(ValueFilter)
    QML_ADDED_IN_VERSION(6, 10)

public:
    explicit QQmlValueFilter(QObject *parent = nullptr);
    ~QQmlValueFilter() = default;

    const QVariant& value() const;
    void setValue(const QVariant& value);
    void resetValue();

    bool filterAcceptsRowInternal(int row, const QModelIndex& sourceIndex, const QQmlSortFilterProxyModel *) const override;

Q_SIGNALS:
    void valueChanged();

private:
    Q_DECLARE_PRIVATE(QQmlValueFilter)
};

class QQmlValueFilterPrivate : public QQmlRoleFilterPrivate
{
    Q_DECLARE_PUBLIC (QQmlValueFilter)

public:
    QVariant m_value;
};

QT_END_NAMESPACE

#endif // QQMLVALUEFILTER_P_H
