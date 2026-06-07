// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLSORTFILTERPROXYMODEL_H
#define QQMLSORTFILTERPROXYMODEL_H

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

#include <QtQml/private/qqmlcustomparser_p.h>
#include <QtQmlModels/private/qtqmlmodelsglobal_p.h>
#include <QtQmlModels/private/qqmlsorterbase_p.h>
#include <QtQmlModels/private/qqmlfilterbase_p.h>
#include <QtQmlModels/private/qsortfilterproxymodelhelper_p.h>

QT_REQUIRE_CONFIG(qml_sfpm_model);

QT_BEGIN_NAMESPACE

class QQmlFilterBase;
class QQmlFilterCompositor;
class QQmlSorterBase;
class QQmlSorterCompositor;
class QQmlSortFilterProxyModelPrivate;
class QQmlSortFilterProxyModelLessThan;
class QQmlSortFilterProxyModelGreaterThan;

class Q_QMLMODELS_EXPORT QQmlSortFilterProxyModel : public QAbstractProxyModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(QQmlListProperty<QQmlFilterBase> filters READ filters NOTIFY filtersChanged FINAL)
    Q_PROPERTY(QQmlListProperty<QQmlSorterBase> sorters READ sorters NOTIFY sortersChanged FINAL)
    Q_PROPERTY(QVariant model READ model WRITE setModel NOTIFY modelChanged FINAL)
    Q_PROPERTY(bool dynamicSortFilter READ dynamicSortFilter WRITE setDynamicSortFilter NOTIFY dynamicSortFilterChanged FINAL)
    Q_PROPERTY(bool recursiveFiltering READ recursiveFiltering WRITE setRecursiveFiltering NOTIFY recursiveFilteringChanged FINAL)
    Q_PROPERTY(bool autoAcceptChildRows READ autoAcceptChildRows WRITE setAutoAcceptChildRows NOTIFY autoAcceptChildRowsChanged FINAL)

    QML_NAMED_ELEMENT(SortFilterProxyModel)
    QML_ADDED_IN_VERSION(6, 10)

public:
    explicit QQmlSortFilterProxyModel(QObject *parent = nullptr);
    ~QQmlSortFilterProxyModel() override;

    // Provides configured filters in this model
    QQmlListProperty<QQmlFilterBase> filters();
    // Provides configured sorters in this model
    QQmlListProperty<QQmlSorterBase> sorters();

    bool dynamicSortFilter() const;
    void setDynamicSortFilter(const bool enabled);

    bool recursiveFiltering() const;
    void setRecursiveFiltering(const bool enabled);

    bool autoAcceptChildRows() const;
    void setAutoAcceptChildRows(const bool enabled);

    QVariant model() const;
    void setModel(QVariant &sourceModel);

    Q_INVOKABLE void invalidate();
    Q_INVOKABLE void invalidateSorter();
    Q_INVOKABLE void setPrimarySorter(QQmlSorterBase *sorter);

    Q_INVOKABLE QModelIndex mapToSource(const QModelIndex& proxyIndex) const override;
    Q_INVOKABLE QModelIndex mapFromSource(const QModelIndex& sourceIndex) const override;

    // Reimplemented methods
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    QModelIndex sibling(int row, int column, const QModelIndex &idx) const override;

    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index,  int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool setHeaderData(int section, Qt::Orientation orientation,
                       const QVariant &value, int role = Qt::EditRole) override;

    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool insertColumns(int column, int count, const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool removeColumns(int column, int count, const QModelIndex &parent = QModelIndex()) override;

    QItemSelection mapSelectionToSource(const QItemSelection &proxySelection) const override;
    QItemSelection mapSelectionFromSource(const QItemSelection &sourceSelection) const override;

    // Internal methods
    void setPrimarySortColumn(const int column);
    void setPrimarySortOrder(const Qt::SortOrder sortOrder);
    QVariant sourceData(const QModelIndex &sourceIndex, int role) const;
    QHash<int, QByteArray> roleNames() const override;
    int itemRoleForName(const QString& roleName) const;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const ;
    bool filterAcceptsColumn(int sourceColumn, const QModelIndex& sourceParent) const ;
    bool lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const ;
    void sort(int column = 0, Qt::SortOrder sortOrder = Qt::AscendingOrder) override;

private:
    void classBegin() override {};
    void componentComplete() override;
    void setSourceModel(QAbstractItemModel *sourceModel) override;
    void invalidateFilter();

Q_SIGNALS:
    void dynamicSortFilterChanged();
    void recursiveFilteringChanged();
    void autoAcceptChildRowsChanged();
    void filtersChanged();
    void sortersChanged();
    void modelChanged();
    void primarySorterChanged();

private:
    Q_DISABLE_COPY(QQmlSortFilterProxyModel)
    Q_DECLARE_PRIVATE(QQmlSortFilterProxyModel)

    friend class QQmlSortFilterProxyModelLessThan;
    friend class QQmlSortFilterProxyModelGreaterThan;
};

QT_END_NAMESPACE

#endif // QQMLSORTFILTERPROXYMODEL_H
