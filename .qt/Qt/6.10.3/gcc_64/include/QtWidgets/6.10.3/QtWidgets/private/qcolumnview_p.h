// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QCOLUMNVIEW_P_H
#define QCOLUMNVIEW_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "qcolumnview.h"

#include <private/qabstractitemview_p.h>

#include <QtCore/qabstractitemmodel.h>
#if QT_CONFIG(animation)
#include <QtCore/qpropertyanimation.h>
#endif
#include <QtWidgets/qabstractitemdelegate.h>
#include <QtWidgets/qabstractitemview.h>
#include <QtWidgets/qstyleditemdelegate.h>
#include <qlistview.h>
#include <qevent.h>
#include <qscrollbar.h>

#include <vector>

QT_REQUIRE_CONFIG(columnview);

QT_BEGIN_NAMESPACE

class QColumnViewPreviewColumn : public QAbstractItemView {

public:
    explicit QColumnViewPreviewColumn(QWidget *parent) : QAbstractItemView(parent), previewWidget(nullptr) {
    }

    void setPreviewWidget(QWidget *widget) {
        previewWidget = widget;
        setMinimumWidth(previewWidget->minimumWidth());
    }

    void resizeEvent(QResizeEvent * event) override{
        if (!previewWidget)
            return;
        previewWidget->resize(
                qMax(previewWidget->minimumWidth(), event->size().width()),
                previewWidget->height());
        QSize p = viewport()->size();
        QSize v = previewWidget->size();
        horizontalScrollBar()->setRange(0, v.width() - p.width());
        horizontalScrollBar()->setPageStep(p.width());
        verticalScrollBar()->setRange(0, v.height() - p.height());
        verticalScrollBar()->setPageStep(p.height());

        QAbstractScrollArea::resizeEvent(event);
    }

    void scrollContentsBy(int dx, int dy) override
    {
        if (!previewWidget)
            return;
        scrollDirtyRegion(dx, dy);
        viewport()->scroll(dx, dy);

        QAbstractItemView::scrollContentsBy(dx, dy);
    }

    QRect visualRect(const QModelIndex &) const override
    {
        return QRect();
    }
    void scrollTo(const QModelIndex &, ScrollHint) override
    {
    }
    QModelIndex indexAt(const QPoint &) const override
    {
        return QModelIndex();
    }
    QModelIndex moveCursor(CursorAction, Qt::KeyboardModifiers) override
    {
        return QModelIndex();
    }
    int horizontalOffset () const override {
        return 0;
    }
    int verticalOffset () const override {
        return 0;
    }
    QRegion visualRegionForSelection(const QItemSelection &) const override
    {
        return QRegion();
    }
    bool isIndexHidden(const QModelIndex &) const override
    {
        return false;
    }
    void setSelection(const QRect &, QItemSelectionModel::SelectionFlags) override
    {
    }
private:
    QWidget *previewWidget;
};

class Q_AUTOTEST_EXPORT QColumnViewPrivate : public QAbstractItemViewPrivate
{
    Q_DECLARE_PUBLIC(QColumnView)

public:
    QColumnViewPrivate();
    ~QColumnViewPrivate();
    void initialize();
    void clearConnections();

    QAbstractItemView *createColumn(const QModelIndex &index, bool show);

    void updateScrollbars();
    void closeColumns(const QModelIndex &parent = QModelIndex(), bool build = false);
    void disconnectView(QAbstractItemView *view);
    void doLayout();
    void setPreviewWidget(QWidget *widget);
    void checkColumnCreation(const QModelIndex &parent);


    void gripMoved(int offset);
    void changeCurrentColumn();
    void clicked(const QModelIndex &index);
    void columnsInserted(const QModelIndex &parent, int start, int end) override;

    QList<QAbstractItemView*> columns;
    QList<int> columnSizes; // used during init and corner moving
    bool showResizeGrips;
    int offset;
#if QT_CONFIG(animation)
    QPropertyAnimation currentAnimation;
    QMetaObject::Connection animationConnection;
#endif
    std::vector<QMetaObject::Connection> gripConnections;
    using ViewConnections = std::vector<QMetaObject::Connection>;
    QHash<QAbstractItemView *, ViewConnections> viewConnections;

    QWidget *previewWidget;
    QAbstractItemView *previewColumn;
};

/*!
 * This is a delegate that will paint the triangle
 */
class QColumnViewDelegate : public QStyledItemDelegate
{

public:
    explicit QColumnViewDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}
    ~QColumnViewDelegate() {}

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
};

QT_END_NAMESPACE

#endif //QCOLUMNVIEW_P_H
