// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKGRIDVIEW_P_H
#define QQUICKGRIDVIEW_P_H

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

#include <QtQuick/private/qtquickglobal_p.h>

QT_REQUIRE_CONFIG(quick_gridview);

#include "qquickitemview_p.h"

QT_BEGIN_NAMESPACE

class QQuickGridViewAttached;
class QQuickGridViewPrivate;
class Q_QUICK_EXPORT QQuickGridView : public QQuickItemView
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickGridView)

    Q_PROPERTY(Flow flow READ flow WRITE setFlow NOTIFY flowChanged)
    Q_PROPERTY(qreal cellWidth READ cellWidth WRITE setCellWidth NOTIFY cellWidthChanged)
    Q_PROPERTY(qreal cellHeight READ cellHeight WRITE setCellHeight NOTIFY cellHeightChanged)

    Q_PROPERTY(SnapMode snapMode READ snapMode WRITE setSnapMode NOTIFY snapModeChanged)

    Q_CLASSINFO("DefaultProperty", "data")
    QML_NAMED_ELEMENT(GridView)
    QML_ADDED_IN_VERSION(2, 0)
    QML_ATTACHED(QQuickGridViewAttached)

public:
    enum Flow {
        FlowLeftToRight = LeftToRight,
        FlowTopToBottom = TopToBottom
    };
    Q_ENUM(Flow)

    QQuickGridView(QQuickItem *parent=nullptr);

    void setHighlightFollowsCurrentItem(bool) override;
    void setHighlightMoveDuration(int) override;

    Flow flow() const;
    void setFlow(Flow);

    qreal cellWidth() const;
    void setCellWidth(qreal);

    qreal cellHeight() const;
    void setCellHeight(qreal);

    enum SnapMode { NoSnap, SnapToRow, SnapOneRow };
    Q_ENUM(SnapMode)
    SnapMode snapMode() const;
    void setSnapMode(SnapMode mode);

    static QQuickGridViewAttached *qmlAttachedProperties(QObject *);

public Q_SLOTS:
    void moveCurrentIndexUp();
    void moveCurrentIndexDown();
    void moveCurrentIndexLeft();
    void moveCurrentIndexRight();

Q_SIGNALS:
    void cellWidthChanged();
    void cellHeightChanged();
    void highlightMoveDurationChanged();
    void flowChanged();
    void snapModeChanged();

protected:
    void viewportMoved(Qt::Orientations) override;
    void keyPressEvent(QKeyEvent *) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void initItem(int index, QObject *item) override;
};

class QQuickGridViewAttached : public QQuickItemViewAttached
{
    Q_OBJECT
    Q_PROPERTY(QQuickGridView *view READ view NOTIFY viewChanged FINAL)
public:
    QQuickGridViewAttached(QObject *parent)
        : QQuickItemViewAttached(parent) {}
    ~QQuickGridViewAttached() {}
    QQuickGridView *view() const { return m_view; }
    void setView(QQuickGridView *view) {
        if (view != m_view) {
            m_view = view;
            Q_EMIT viewChanged();
        }
    }
private:
    QPointer<QQuickGridView> m_view;
};


QT_END_NAMESPACE

#endif // QQUICKGRIDVIEW_P_H
