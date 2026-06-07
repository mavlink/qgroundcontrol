// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLSORTERBASE_H
#define QQMLSORTERBASE_H

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

#include <QtQml/qqml.h>
#include <QtCore/QObject>
#include <QtCore/QAbstractItemModel>
#include <QtQmlModels/private/qtqmlmodelsglobal_p.h>
#include <QtQml/private/qqmlcustomparser_p.h>

QT_BEGIN_NAMESPACE

class QQmlSortFilterProxyModel;
class QQmlSorterBasePrivate;

class Q_QMLMODELS_EXPORT QQmlSorterBase : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged FINAL)
    Q_PROPERTY(Qt::SortOrder sortOrder READ sortOrder WRITE setSortOrder NOTIFY sortOrderChanged FINAL)
    Q_PROPERTY(int priority READ priority WRITE setPriority NOTIFY priorityChanged FINAL)
    Q_PROPERTY(int column READ column WRITE setColumn NOTIFY columnChanged FINAL)
    QML_NAMED_ELEMENT(SorterBase)
    QML_UNCREATABLE("")
    QML_ADDED_IN_VERSION(6, 10)

public:
    explicit QQmlSorterBase(QQmlSorterBasePrivate *privObj, QObject *parent = nullptr);
    virtual ~QQmlSorterBase() = default;

    bool enabled() const;
    void setEnabled(const bool enabled);

    Qt::SortOrder sortOrder() const;
    void setSortOrder(const Qt::SortOrder sortOrder);

    int priority() const;
    void setPriority(const int priority);

    int column() const;
    void setColumn(const int column);

    virtual QPartialOrdering compare(const QModelIndex&, const QModelIndex&, const QQmlSortFilterProxyModel *) const = 0;
    virtual void update(const QQmlSortFilterProxyModel *) { /* do nothing */ }

Q_SIGNALS:
    void enabledChanged();
    void sortOrderChanged();
    void priorityChanged();
    void columnChanged();
    void invalidateModel();
    void invalidateCache(QQmlSorterBase *filter);

public slots:
    void invalidate(bool updateCache = true);

private:
    Q_DECLARE_PRIVATE(QQmlSorterBase)
};

class QQmlSorterBasePrivate: public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQmlSorterBase)

public:
    QQmlSorterBasePrivate() = default;
    virtual ~QQmlSorterBasePrivate() = default;

    bool m_enabled = true;
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;
    int m_sorterPriority = std::numeric_limits<int>::max();
    int m_sortColumn = 0;
};

QT_END_NAMESPACE

#endif // QQMLSORTERBASE_H
