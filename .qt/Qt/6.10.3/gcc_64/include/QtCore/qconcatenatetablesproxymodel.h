// Copyright (C) 2016 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author David Faure <david.faure@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QCONCATENATEROWSPROXYMODEL_H
#define QCONCATENATEROWSPROXYMODEL_H

#include <QtCore/qabstractitemmodel.h>

QT_REQUIRE_CONFIG(concatenatetablesproxymodel);

QT_BEGIN_NAMESPACE

class QConcatenateTablesProxyModelPrivate;

class Q_CORE_EXPORT QConcatenateTablesProxyModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit QConcatenateTablesProxyModel(QObject *parent = nullptr);
    ~QConcatenateTablesProxyModel();

    QList<QAbstractItemModel *> sourceModels() const;
    Q_SCRIPTABLE void addSourceModel(QAbstractItemModel *sourceModel);
    Q_SCRIPTABLE void removeSourceModel(QAbstractItemModel *sourceModel);

    QModelIndex mapFromSource(const QModelIndex &sourceIndex) const;
    QModelIndex mapToSource(const QModelIndex &proxyIndex) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QMap<int, QVariant> itemData(const QModelIndex &proxyIndex) const override;
    bool setItemData(const QModelIndex &index, const QMap<int, QVariant> &roles) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
    QSize span(const QModelIndex &index) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    Q_DECLARE_PRIVATE(QConcatenateTablesProxyModel)
    Q_DISABLE_COPY(QConcatenateTablesProxyModel)
};

QT_END_NAMESPACE

#endif // QCONCATENATEROWSPROXYMODEL_H
