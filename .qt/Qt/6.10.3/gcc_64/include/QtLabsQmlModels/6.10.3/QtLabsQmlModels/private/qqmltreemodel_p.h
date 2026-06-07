// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLTREEMODEL_P_H
#define QQMLTREEMODEL_P_H

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

#include "qqmlabstractcolumnmodel_p.h"

#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qvariant.h>

#include <memory>
#include <vector>

QT_REQUIRE_CONFIG(qml_tree_model);

class tst_QQmlTreeModel;

QT_BEGIN_NAMESPACE

class QQmlTreeRow;

class Q_LABSQMLMODELS_EXPORT QQmlTreeModel : public QQmlAbstractColumnModel
{
    Q_OBJECT
    Q_PROPERTY(QVariant rows READ rows WRITE setRows NOTIFY rowsChanged FINAL)
    QML_NAMED_ELEMENT(TreeModel)
    QML_ADDED_IN_VERSION(6, 10)

public:
    Q_DISABLE_COPY_MOVE(QQmlTreeModel)

    explicit QQmlTreeModel(QObject *parent = nullptr);
    ~QQmlTreeModel() override;

    QVariant rows() const;
    void setRows(const QVariant &rows);

    Q_INVOKABLE void appendRow(QModelIndex parent, const QVariant &row);
    Q_INVOKABLE void appendRow(const QVariant &row);
    Q_INVOKABLE void clear();
    Q_INVOKABLE QVariant getRow(const QModelIndex &index) const;
    Q_INVOKABLE void removeRow(QModelIndex index);
    Q_INVOKABLE void setRow(QModelIndex index, const QVariant &rowData);

    Q_INVOKABLE QModelIndex index(const std::vector<int> &rowIndex, int column);

    //AbstractItemModel interface
    QModelIndex index(int row, int column, const QModelIndex &parent = {}) const override;
    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QModelIndex parent(const QModelIndex &index) const override;

protected:
    QVariant firstRow() const override;
    void setInitialRows() override;

private:
    QQmlTreeRow *getPointerToTreeRow(QModelIndex &index, const std::vector<int> &rowIndex) const;

    int treeSize() const;
    friend class ::tst_QQmlTreeModel;

    void setRowsPrivate(const QVariantList &rowsAsVariantList);
    QVariant dataPrivate(const QModelIndex &index, const QString &roleName) const override;
    void setDataPrivate(const QModelIndex &index, const QString &roleName, QVariant value) override;

    bool validateNewRow(QLatin1StringView functionName, const QVariant &row,
        NewRowOperationFlag = OtherOperation) const override;

    std::vector<std::unique_ptr<QQmlTreeRow>> mRows;

    QVariantList mInitialRows;
};

QT_END_NAMESPACE

#endif // QQMLTREEMODEL_P_H

