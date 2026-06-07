// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLSORTERCOMPOSITOR_H
#define QQMLSORTERCOMPOSITOR_H

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
class QQmlSorterCompositorPrivate;

class QQmlSorterCompositor: public QQmlSorterBase
{
    Q_OBJECT
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(6, 10)

public:
    explicit QQmlSorterCompositor(QObject *parent = nullptr);
    ~QQmlSorterCompositor() override;

    QList<QQmlSorterBase *> sorters();
    QQmlListProperty<QQmlSorterBase> sortersListProperty();

    static void append(QQmlListProperty<QQmlSorterBase> *sorterComp, QQmlSorterBase *sorter);
    static qsizetype count(QQmlListProperty<QQmlSorterBase> *sorterComp);
    static QQmlSorterBase* at(QQmlListProperty<QQmlSorterBase> *sorterComp, qsizetype index);
    static void clear(QQmlListProperty<QQmlSorterBase> *sorterComp);

    QPartialOrdering compare(const QModelIndex &, const QModelIndex &, const QQmlSortFilterProxyModel *) const override { return QPartialOrdering::Unordered; };
    bool lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight, const QQmlSortFilterProxyModel *proxyModel) const;
    void updateSorters();
    void updateEffectiveSorters();

private:
    void append(QQmlSorterBase *sorter);
    qsizetype count();
    QQmlSorterBase* at(qsizetype index);
    void clear();

public slots:
    void updateCache();

private:
    Q_DECLARE_PRIVATE(QQmlSorterCompositor)
};

class QQmlSorterCompositorPrivate: public QQmlSorterBasePrivate
{
    Q_DECLARE_PUBLIC(QQmlSorterCompositor)

public:
    void init();
    void setPrimarySorter(QQmlSorterBase *sorter);
    QPointer<QQmlSorterBase> primarySorter() const { return m_primarySorter; }
    void resetPrimarySorter();

    // Holds sorters in the same order as declared in the qml
    QList<QQmlSorterBase *> m_sorters;
    // Holds effective sorters that will be evaluated with the
    // model content
    QList<QQmlSorterBase *> m_effectiveSorters;
    QQmlSortFilterProxyModel *m_sfpmModel = nullptr;
    QPointer<QQmlSorterBase> m_primarySorter;
};

QT_END_NAMESPACE

#endif // QQMLSORTERCOMPOSITOR_H
