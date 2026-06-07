// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLMODELINDEXVALUETYPE_P_H
#define QQMLMODELINDEXVALUETYPE_P_H

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

#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qitemselectionmodel.h>
#include <QtQml/qqml.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

struct QQmlModelIndexValueType
{
    QModelIndex v;

    Q_PROPERTY(int row READ row CONSTANT FINAL)
    Q_PROPERTY(int column READ column CONSTANT FINAL)
    Q_PROPERTY(QModelIndex parent READ parent FINAL)
    Q_PROPERTY(bool valid READ isValid CONSTANT FINAL)
    Q_PROPERTY(QAbstractItemModel *model READ model CONSTANT FINAL)
    Q_PROPERTY(quint64 internalId READ internalId CONSTANT FINAL)
    Q_GADGET
    QML_ANONYMOUS
    QML_EXTENDED(QQmlModelIndexValueType)
    QML_FOREIGN(QModelIndex)
    QML_ADDED_IN_VERSION(2, 0)

public:
    Q_INVOKABLE QString toString() const
    { return QLatin1String("QModelIndex") + propertiesString(v); }

    Q_REVISION(6, 7) Q_INVOKABLE QVariant data(int role = Qt::DisplayRole) const
    { return v.data(role); }

    inline int row() const noexcept { return v.row(); }
    inline int column() const noexcept { return v.column(); }
    inline QModelIndex parent() const { return v.parent(); }
    inline bool isValid() const noexcept { return v.isValid(); }
    inline QAbstractItemModel *model() const noexcept
    { return const_cast<QAbstractItemModel *>(v.model()); }
    quint64 internalId() const { return v.internalId(); }

    static QString propertiesString(const QModelIndex &idx);

    static QPersistentModelIndex toPersistentModelIndex(const QModelIndex &index)
    { return QPersistentModelIndex(index); }

    operator QModelIndex() const { return v; }
};

struct QQmlPersistentModelIndexValueType
{
    QPersistentModelIndex v;

    Q_PROPERTY(int row READ row FINAL)
    Q_PROPERTY(int column READ column FINAL)
    Q_PROPERTY(QModelIndex parent READ parent FINAL)
    Q_PROPERTY(bool valid READ isValid FINAL)
    Q_PROPERTY(QAbstractItemModel *model READ model FINAL)
    Q_PROPERTY(quint64 internalId READ internalId FINAL)
    Q_GADGET
    QML_ANONYMOUS
    QML_EXTENDED(QQmlPersistentModelIndexValueType)
    QML_FOREIGN(QPersistentModelIndex)
    QML_ADDED_IN_VERSION(2, 0)

public:
    Q_INVOKABLE QString toString() const
    { return QLatin1String("QPersistentModelIndex") + QQmlModelIndexValueType::propertiesString(v); }

    Q_REVISION(6, 7) Q_INVOKABLE QVariant data(int role = Qt::DisplayRole) const
    { return v.data(role); }

    inline int row() const { return v.row(); }
    inline int column() const { return v.column(); }
    inline QModelIndex parent() const { return v.parent(); }
    inline bool isValid() const { return v.isValid(); }
    inline QAbstractItemModel *model() const { return const_cast<QAbstractItemModel *>(v.model()); }
    inline quint64 internalId() const { return v.internalId(); }

    operator QPersistentModelIndex() const { return v; }
};

struct QQmlItemSelectionRangeValueType
{
    QItemSelectionRange v;

    Q_PROPERTY(int top READ top FINAL)
    Q_PROPERTY(int left READ left FINAL)
    Q_PROPERTY(int bottom READ bottom FINAL)
    Q_PROPERTY(int right READ right FINAL)
    Q_PROPERTY(int width READ width FINAL)
    Q_PROPERTY(int height READ height FINAL)
    Q_PROPERTY(QPersistentModelIndex topLeft READ topLeft FINAL)
    Q_PROPERTY(QPersistentModelIndex bottomRight READ bottomRight FINAL)
    Q_PROPERTY(QModelIndex parent READ parent FINAL)
    Q_PROPERTY(bool valid READ isValid FINAL)
    Q_PROPERTY(bool empty READ isEmpty FINAL)
    Q_PROPERTY(QAbstractItemModel *model READ model FINAL)
    Q_GADGET
    QML_ANONYMOUS
    QML_EXTENDED(QQmlItemSelectionRangeValueType)
    QML_FOREIGN(QItemSelectionRange)
    QML_ADDED_IN_VERSION(2, 0)

public:
    Q_INVOKABLE QString toString() const;
    Q_INVOKABLE inline bool contains(const QModelIndex &index) const
    { return v.contains(index); }
    Q_INVOKABLE inline bool contains(int row, int column, const QModelIndex &parentIndex) const
    { return v.contains(row, column, parentIndex); }
    Q_INVOKABLE inline bool intersects(const QItemSelectionRange &other) const
    { return v.intersects(other); }
    Q_INVOKABLE QItemSelectionRange intersected(const QItemSelectionRange &other) const
    { return v.intersected(other); }

    inline int top() const { return v.top(); }
    inline int left() const { return v.left(); }
    inline int bottom() const { return v.bottom(); }
    inline int right() const { return v.right(); }
    inline int width() const { return v.width(); }
    inline int height() const { return v.height(); }
    inline QPersistentModelIndex &topLeft() const { return const_cast<QPersistentModelIndex &>(v.topLeft()); }
    inline QPersistentModelIndex &bottomRight() const { return const_cast<QPersistentModelIndex &>(v.bottomRight()); }
    inline QModelIndex parent() const { return v.parent(); }
    inline QAbstractItemModel *model() const { return const_cast<QAbstractItemModel *>(v.model()); }
    inline bool isValid() const { return v.isValid(); }
    inline bool isEmpty() const { return v.isEmpty(); }

    operator QItemSelectionRange() const { return v; }
};

struct QModelIndexListForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_SEQUENTIAL_CONTAINER(QModelIndex)
    QML_FOREIGN(QModelIndexList)
    QML_ADDED_IN_VERSION(2, 0)
};

struct QModelIndexStdVectorForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_SEQUENTIAL_CONTAINER(QModelIndex)
    QML_FOREIGN(std::vector<QModelIndex>)
    QML_ADDED_IN_VERSION(2, 0)
};

struct QItemSelectionForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_SEQUENTIAL_CONTAINER(QItemSelectionRange)
    QML_FOREIGN(QItemSelection)
    QML_ADDED_IN_VERSION(2, 0)
};

#undef QLISTVALUETYPE_INVOKABLE_API

QT_END_NAMESPACE

#endif // QQMLMODELINDEXVALUETYPE_P_H

