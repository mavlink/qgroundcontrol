// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLFILTERCOMPOSITOR_P_H
#define QQMLFILTERCOMPOSITOR_P_H

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
class QQmlFilterCompositorPrivate;

class QQmlFilterCompositor: public QQmlFilterBase
{
    Q_OBJECT
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(6, 10)

public:
    explicit QQmlFilterCompositor(QObject *parent = nullptr);
    ~QQmlFilterCompositor() override;

    QList<QQmlFilterBase *> filters();
    QQmlListProperty<QQmlFilterBase> filtersListProperty();

    bool filterAcceptsRowInternal(int, const QModelIndex&, const QQmlSortFilterProxyModel *) const override;
    bool filterAcceptsColumnInternal(int, const QModelIndex&, const QQmlSortFilterProxyModel *) const override;
    void updateFilters();

    static void append(QQmlListProperty<QQmlFilterBase> *filterComp, QQmlFilterBase *filter);
    static qsizetype count(QQmlListProperty<QQmlFilterBase> *filterComp);
    static QQmlFilterBase* at(QQmlListProperty<QQmlFilterBase> *filterComp, qsizetype index);
    static void clear(QQmlListProperty<QQmlFilterBase> *filterComp);

private:
    void append(QQmlFilterBase *);
    qsizetype count();
    QQmlFilterBase* at(qsizetype);
    void clear();

public slots:
    void updateCache();

private:
    Q_DECLARE_PRIVATE(QQmlFilterCompositor)
};

class QQmlFilterCompositorPrivate: public QQmlFilterBasePrivate
{
    Q_DECLARE_PUBLIC(QQmlFilterCompositor)

public:
    void init();
    // Holds filters in the same order as declared in the qml
    QList<QQmlFilterBase *> m_filters;
    // Holds effective filters that will be evaluated with the
    // model content
    QList<QQmlFilterBase *> m_effectiveFilters;
    QQmlSortFilterProxyModel *m_sfpmModel = nullptr;
};

QT_END_NAMESPACE

#endif // QQMLFILTERCOMPOSITOR_P_H
