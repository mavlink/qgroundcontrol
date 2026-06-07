// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLABSTRACTCOLUMNMODEL_P_H
#define QQMLABSTRACTCOLUMNMODEL_P_H

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

#include "qqmltablemodelcolumn_p.h"

#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE

class Q_LABSQMLMODELS_EXPORT QQmlAbstractColumnModel : public QAbstractItemModel, public QQmlParserStatus
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(int columnCount READ columnCount NOTIFY columnCountChanged FINAL)
    Q_PROPERTY(QQmlListProperty<QQmlTableModelColumn> columns READ columns CONSTANT FINAL)
    Q_INTERFACES(QQmlParserStatus)
    Q_CLASSINFO("DefaultProperty", "columns")

public:
    Q_DISABLE_COPY_MOVE(QQmlAbstractColumnModel)

    explicit QQmlAbstractColumnModel(QObject *parent = nullptr);
    ~QQmlAbstractColumnModel() override = default;

    QQmlListProperty<QQmlTableModelColumn> columns();

    static void columns_append(QQmlListProperty<QQmlTableModelColumn> *property, QQmlTableModelColumn *value);
    static qsizetype columns_count(QQmlListProperty<QQmlTableModelColumn> *property);
    static QQmlTableModelColumn *columns_at(QQmlListProperty<QQmlTableModelColumn> *property, qsizetype index);
    static void columns_clear(QQmlListProperty<QQmlTableModelColumn> *property);
    static void columns_replace(QQmlListProperty<QQmlTableModelColumn> *property, qsizetype index, QQmlTableModelColumn *value);
    static void columns_removeLast(QQmlListProperty<QQmlTableModelColumn> *property);

    Q_INVOKABLE QVariant data(const QModelIndex &index, const QString &role) const;
    Q_INVOKABLE QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Q_INVOKABLE bool setData(const QModelIndex &index, const QVariant &value, const QString &role);
    Q_INVOKABLE bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::DisplayRole) override;

    QHash<int, QByteArray> roleNames() const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

Q_SIGNALS:
    void columnCountChanged();
    void rowsChanged();

protected:
    void classBegin() override;
    void componentComplete() override;

    virtual QVariant firstRow() const = 0;
    virtual void setInitialRows() = 0;

    virtual QVariant dataPrivate(const QModelIndex &index, const QString &roleName) const = 0;
    virtual void setDataPrivate(const QModelIndex &index, const QString &roleName, QVariant value) = 0;

    enum class ColumnRole : quint8
    {
        StringRole,
        FunctionRole
    };

    class ColumnRoleMetadata
    {
    public:
        ColumnRoleMetadata();
        ColumnRoleMetadata(ColumnRole role, QString name, int type, QString typeName);

        bool isValid() const;

        ColumnRole columnRole = ColumnRole::FunctionRole;
        QString name;
        int type = QMetaType::UnknownType;
        QString typeName;
    };

    struct ColumnMetadata
    {
        // Key = role name that will be made visible to the delegate
        // Value = metadata about that role, including actual name in the model data, type, etc.
        QHash<QString, ColumnRoleMetadata> roles;
    };

    ColumnRoleMetadata fetchColumnRoleData(const QString &roleNameKey, QQmlTableModelColumn *tableModelColumn, int columnIndex) const;
    void fetchColumnMetadata();

    enum NewRowOperationFlag {
        OtherOperation, // insert(), set(), etc.
        SetRowsOperation,
        AppendOperation
    };

    bool validateRowType(QLatin1StringView functionName, const QVariant &row) const;
    virtual bool validateNewRow(QLatin1StringView functionName, const QVariant &row,
                                NewRowOperationFlag operation  = OtherOperation) const;

    QList<QQmlTableModelColumn *> mColumns;

    bool mComponentCompleted = false;
    int mColumnCount = 0;
    // Each entry contains information about the properties of the column at that index.
    QVector<ColumnMetadata> mColumnMetadata;
    // key = property index (0 to number of properties across all columns)
    // value = role name
    QHash<int, QByteArray> mRoleNames;
};

QT_END_NAMESPACE

#endif // QQMLABSTRACTCOLUMNMODEL_P_H
