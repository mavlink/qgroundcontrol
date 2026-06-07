// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLFUNCTIONSORTER_P_H
#define QQMLFUNCTIONSORTER_P_H

//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtQmlModels/private/qqmlrolesorter_p.h>
#include <QtQmlModels/private/qqmlsorterbase_p.h>

QT_BEGIN_NAMESPACE

class QQmlSortFilterProxyModel;
class QQmlFunctionSorterPrivate;

class Q_QMLMODELS_EXPORT QQmlFunctionSorter : public QQmlSorterBase, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    QML_NAMED_ELEMENT(FunctionSorter)
    QML_ADDED_IN_VERSION(6, 10)

public:
    explicit QQmlFunctionSorter(QObject *parent = nullptr);
    ~QQmlFunctionSorter() override;

    virtual QPartialOrdering compare(const QModelIndex&, const QModelIndex&, const QQmlSortFilterProxyModel *) const override;

private:
    void classBegin() override {};
    void componentComplete() override;

private:
    Q_DECLARE_PRIVATE(QQmlFunctionSorter)
};

class QQmlFunctionSorterPrivate : public QQmlRoleSorterPrivate
{
    Q_DECLARE_PUBLIC (QQmlFunctionSorter)

public:
    QMetaMethod m_method;
    mutable QVariant m_lhsParameterData;
    mutable QVariant m_rhsParameterData;
};

QT_END_NAMESPACE

#endif // QQMLFUNCTIONSORTER_P_H
