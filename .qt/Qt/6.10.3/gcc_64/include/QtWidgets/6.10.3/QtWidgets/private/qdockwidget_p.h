// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QDYNAMICDOCKWIDGET_P_H
#define QDYNAMICDOCKWIDGET_P_H

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

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "QtWidgets/qstyleoption.h"
#include "private/qwidget_p.h"
#include "QtWidgets/qboxlayout.h"
#include "QtWidgets/qdockwidget.h"

#if QT_CONFIG(tabwidget)
#  include "QtWidgets/qtabwidget.h"
#endif

QT_REQUIRE_CONFIG(dockwidget);

QT_BEGIN_NAMESPACE

class QGridLayout;
class QWidgetResizeHandler;
class QDockWidgetTitleButton;
class QSpacerItem;
class QDockWidgetItem;

class QDockWidgetPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QDockWidget)

    struct DragState {
        QPoint pressPos;
        QPoint globalPressPos;
        QPoint widgetInitialPos;
        bool dragging;
        QLayoutItem *widgetItem;
        bool ownWidgetItem;
        bool nca;
        bool ctrlDrag;
    };

public:
    enum class DragScope {
        Group,
        Widget
    };

    enum class EndDragMode {
        LocationChange,
        Abort
    };

    enum class WindowState {
        Unplug = 0x01,
        Floating = 0x02,
    };
    Q_DECLARE_FLAGS(WindowStates, WindowState)

    void init();
    void toggleView(bool);
    void toggleTopLevel();

    void updateButtons();
    static Qt::DockWidgetArea toDockWidgetArea(QInternal::DockPosition pos);

#if QT_CONFIG(tabwidget)
    QTabWidget::TabPosition tabPosition = QTabWidget::North;
#endif

    DragState *state = nullptr;

    QDockWidget::DockWidgetFeatures features = QDockWidget::DockWidgetClosable
        | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable;
    Qt::DockWidgetAreas allowedAreas = Qt::AllDockWidgetAreas;

    QFont font;

#ifndef QT_NO_ACTION
    QAction *toggleViewAction = nullptr;
#endif

//    QMainWindow *findMainWindow(QWidget *widget) const;
    QRect undockedGeometry;
    QString fixedWindowTitle;
    QString dockedWindowTitle;

    bool mousePressEvent(QMouseEvent *event);
    bool mouseDoubleClickEvent(QMouseEvent *event);
    bool mouseMoveEvent(QMouseEvent *event);
    bool mouseReleaseEvent(QMouseEvent *event);
    void setWindowState(WindowStates states, const QRect &rect = QRect());
    void nonClientAreaMouseEvent(QMouseEvent *event);
    void initDrag(const QPoint &pos, bool nca);
    void startDrag(DragScope scope);
    void endDrag(EndDragMode mode);
    void moveEvent(QMoveEvent *event);
    void recalculatePressPos(QResizeEvent *event);

    void unplug(const QRect &rect);
    void plug(const QRect &rect);
    void setResizerActive(bool active);
    void setFloating(bool floating);

    bool isAnimating() const;
    bool isTabbed() const;

private:
    QWidgetResizeHandler *resizer = nullptr;
};

class Q_WIDGETS_EXPORT QDockWidgetLayout : public QLayout
{
    Q_OBJECT
public:
    explicit QDockWidgetLayout(QWidget *parent = nullptr);
    ~QDockWidgetLayout();
    void addItem(QLayoutItem *item) override;
    QLayoutItem *itemAt(int index) const override;
    QLayoutItem *takeAt(int index) override;
    int count() const override;

    QSize maximumSize() const override;
    QSize minimumSize() const override;
    QSize sizeHint() const override;

    QSize sizeFromContent(const QSize &content, bool floating) const;

    void setGeometry(const QRect &r) override;

    enum Role { Content, CloseButton, FloatButton, TitleBar, RoleCount };
    QWidget *widgetForRole(Role r) const;
    void setWidgetForRole(Role r, QWidget *w);
    QLayoutItem *itemForRole(Role r) const;

    QRect titleArea() const { return _titleArea; }

    int minimumTitleWidth() const;
    int titleHeight() const;
    void updateMaxSize();
    static bool wmSupportsNativeWindowDeco();
    bool nativeWindowDeco() const;
    bool nativeWindowDeco(bool floating) const;

    void setVerticalTitleBar(bool b);

    bool verticalTitleBar;

private:
    QList<QLayoutItem *> item_list;
    QRect _titleArea;
};

/* The size hints of a QDockWidget will depend on whether it is docked or not.
   This layout item always returns the size hints as if the dock widget was docked. */

class QDockWidgetItem : public QWidgetItem
{
public:
    QDockWidgetItem(QDockWidget *dockWidget);
    QSize minimumSize() const override;
    QSize maximumSize() const override;
    QSize sizeHint() const override;

private:
    inline QLayoutItem *dockWidgetChildItem() const;
    inline QDockWidgetLayout *dockWidgetLayout() const;
};

inline QLayoutItem *QDockWidgetItem::dockWidgetChildItem() const
{
    if (QDockWidgetLayout *layout = dockWidgetLayout())
        return layout->itemForRole(QDockWidgetLayout::Content);
    return nullptr;
}

inline QDockWidgetLayout *QDockWidgetItem::dockWidgetLayout() const
{
    QWidget *w = widget();
    if (w != nullptr)
        return qobject_cast<QDockWidgetLayout*>(w->layout());
    return nullptr;
}

QT_END_NAMESPACE

#endif // QDYNAMICDOCKWIDGET_P_H
