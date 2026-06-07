// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKHEADERVIEW_P_P_H
#define QQUICKHEADERVIEW_P_P_H

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

#include <QtCore/QAbstractItemModel>
#include <QtCore/QPointer>
#include <QtQuick/private/qquicktableview_p_p.h>
#include <private/qquickheaderview_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICKTEMPLATES2_EXPORT QHeaderDataProxyModel : public QAbstractItemModel
{
    Q_OBJECT
    Q_DISABLE_COPY(QHeaderDataProxyModel)
    Q_PROPERTY(QAbstractItemModel *sourceModel READ sourceModel)
public:
    explicit QHeaderDataProxyModel(QObject *parent = nullptr);
    ~QHeaderDataProxyModel();

    void setSourceModel(QAbstractItemModel *newSourceModel);
    QPointer<QAbstractItemModel> sourceModel() const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    QModelIndex sibling(int row, int column, const QModelIndex &idx) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

    inline QVariant variantValue() const;
    inline Qt::Orientation orientation() const;
    inline void setOrientation(Qt::Orientation o);

    QQuickHeaderViewBase *m_headerView = nullptr;

private:
    inline void connectToModel();
    inline void disconnectFromModel();
    QPointer<QAbstractItemModel> m_model = nullptr;
    Qt::Orientation m_orientation = Qt::Horizontal;
};

class QQuickHeaderViewBasePrivate : public QQuickTableViewPrivate
{
    Q_DECLARE_PUBLIC(QQuickHeaderViewBase)
public:
    QQuickHeaderViewBasePrivate();
    ~QQuickHeaderViewBasePrivate();

    static inline QQuickHeaderViewBasePrivate *get(QQuickHeaderViewBase *q) { return q->d_func(); }

    void init();
    Qt::Orientation orientation() const;
    void setOrientation(Qt::Orientation orientation);
    const QPointer<QQuickItem> delegateItemAt(int row, int col) const;
    QVariant modelImpl() const override;
    void setModelImpl(const QVariant &newModel) override;
    void syncModel() override;
    void syncSyncView() override;
    QAbstractItemModel *selectionSourceModel() override;

    // QQuickTableViewPrivate interface
    virtual void initItemCallback(int modelIndex, QObject *item) override;

protected:
    QHeaderDataProxyModel m_headerDataProxyModel;
    struct SectionSize
    {
        int section;
        qreal previousSize;
    };
    QStack<SectionSize> m_hiddenSectionSizes;
    bool m_modelExplicitlySet = false;
    QString m_textRole;

    int logicalRowIndex(const int visualIndex) const final;
    int logicalColumnIndex(const int visualIndex) const final;
    int visualRowIndex(const int logicalIndex) const final;
    int visualColumnIndex(const int logicalIndex) const final;
};

class QQuickHorizontalHeaderViewPrivate : public QQuickHeaderViewBasePrivate
{
    Q_DECLARE_PUBLIC(QQuickHorizontalHeaderView)
public:
    QQuickHorizontalHeaderViewPrivate();
    ~QQuickHorizontalHeaderViewPrivate();

    bool m_movableColumns = false;
};

class QQuickVerticalHeaderViewPrivate : public QQuickHeaderViewBasePrivate
{
    Q_DECLARE_PUBLIC(QQuickVerticalHeaderView)
public:
    QQuickVerticalHeaderViewPrivate();
    ~QQuickVerticalHeaderViewPrivate();

    bool m_movableRows = false;
};

QT_END_NAMESPACE

#endif // QQUICKHEADERVIEW_P_P_H
