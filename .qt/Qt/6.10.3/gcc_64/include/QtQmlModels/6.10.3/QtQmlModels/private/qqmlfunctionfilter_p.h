// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLFUNCTIONFILTER_P_H
#define QQMLFUNCTIONFILTER_P_H

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

#include <QList>
#include <QQmlScriptString>
#include <QtQmlModels/private/qqmlfilterbase_p.h>

QT_BEGIN_NAMESPACE

class QQmlSortFilterProxyModel;
class QQmlFunctionFilterPrivate;

class Q_QMLMODELS_EXPORT QQmlFunctionFilter : public QQmlFilterBase, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    QML_NAMED_ELEMENT(FunctionFilter)
    QML_ADDED_IN_VERSION(6, 10)

public:
    explicit QQmlFunctionFilter(QObject *parent = nullptr);
    ~QQmlFunctionFilter() override;

    bool filterAcceptsRowInternal(int row, const QModelIndex& sourceIndex, const QQmlSortFilterProxyModel *) const override;

private:
    void classBegin() override {};
    void componentComplete() override;

private:
    Q_DECLARE_PRIVATE(QQmlFunctionFilter)
};

class QQmlFunctionFilterPrivate : public QQmlFilterBasePrivate
{
    Q_DECLARE_PUBLIC (QQmlFunctionFilter)

public:
    QMetaMethod m_method;
    mutable QVariant m_parameterData;
};

QT_END_NAMESPACE

#endif // QQMLFUNCTIONFILTER_P_H
