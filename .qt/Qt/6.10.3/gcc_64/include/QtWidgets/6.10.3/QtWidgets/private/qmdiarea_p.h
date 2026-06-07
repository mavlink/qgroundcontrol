// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QMDIAREA_P_H
#define QMDIAREA_P_H

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
#include "qmdiarea.h"
#include "qmdisubwindow.h"

QT_REQUIRE_CONFIG(mdiarea);

#include <QBasicTimer>
#include <QList>
#include <QRect>
#include <QPoint>
#include <QtWidgets/qapplication.h>
#include <private/qmdisubwindow_p.h>
#include <private/qabstractscrollarea_p.h>

#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

namespace QMdi {
class Rearranger
{
public:
    enum Type {
        RegularTiler,
        SimpleCascader,
        IconTiler
    };

    // Rearranges widgets relative to domain.
    virtual void rearrange(QList<QWidget *> &widgets, const QRect &domain) const = 0;
    virtual Type type() const = 0;
    virtual ~Rearranger() {}
};

class RegularTiler : public Rearranger
{
    // Rearranges widgets according to a regular tiling pattern
    // covering the entire domain.
    // Both positions and sizes may change.
    void rearrange(QList<QWidget *> &widgets, const QRect &domain) const override;
    Type type() const override { return Rearranger::RegularTiler; }
};

class SimpleCascader : public Rearranger
{
    // Rearranges widgets according to a simple, regular cascading pattern.
    // Widgets are resized to minimumSize.
    // Both positions and sizes may change.
    void rearrange(QList<QWidget *> &widgets, const QRect &domain) const override;
    Type type() const override { return Rearranger::SimpleCascader; }
};

class IconTiler : public Rearranger
{
    // Rearranges icons (assumed to be the same size) according to a regular
    // tiling pattern filling up the domain from the bottom.
    // Only positions may change.
    void rearrange(QList<QWidget *> &widgets, const QRect &domain) const override;
    Type type() const override { return Rearranger::IconTiler; }
};

class Placer
{
public:
    // Places the rectangle defined by 'size' relative to 'rects' and 'domain'.
    // Returns the position of the resulting rectangle.
    virtual QPoint place(const QSize &size, const QList<QRect> &rects,
                         const QRect &domain) const = 0;
    virtual ~Placer() {}
};

class MinOverlapPlacer : public Placer
{
    QPoint place(const QSize &size, const QList<QRect> &rects, const QRect &domain) const override;
    static int accumulatedOverlap(const QRect &source, const QList<QRect> &rects);
    static QRect findMinOverlapRect(const QList<QRect> &source, const QList<QRect> &rects);
    static QList<QRect> getCandidatePlacements(const QSize &size, const QList<QRect> &rects,
                                               const QRect &domain);
    static QPoint findBestPlacement(const QRect &domain, const QList<QRect> &rects,
                                    QList<QRect> &source);
    static QList<QRect> findNonInsiders(const QRect &domain, QList<QRect> &source);
    static QList<QRect> findMaxOverlappers(const QRect &domain, const QList<QRect> &source);
};
} // namespace QMdi

class QMdiAreaTabBar;
class QMdiAreaPrivate : public QAbstractScrollAreaPrivate
{
    Q_DECLARE_PUBLIC(QMdiArea)
public:
    QMdiAreaPrivate();

    // Variables.
    QMdi::Rearranger *cascader;
    QMdi::Rearranger *regularTiler;
    QMdi::Rearranger *iconTiler;
    QMdi::Placer *placer;
#if QT_CONFIG(rubberband)
    QRubberBand *rubberBand;
#endif
    QMdiAreaTabBar *tabBar;
    QList<QMdi::Rearranger *> pendingRearrangements;
    QList<QPointer<QMdiSubWindow>> pendingPlacements;
    QList<QPointer<QMdiSubWindow>> childWindows;
    QList<int> indicesToActivatedChildren;
    QPointer<QMdiSubWindow> active;
    QPointer<QMdiSubWindow> aboutToBecomeActive;
    QBrush background;
    QMdiArea::WindowOrder activationOrder;
    QMdiArea::AreaOptions options;
    QMdiArea::ViewMode viewMode;
#if QT_CONFIG(tabbar)
    bool documentMode;
    bool tabsClosable;
    bool tabsMovable;
#endif
#if QT_CONFIG(tabwidget)
    QTabWidget::TabShape tabShape;
    QTabWidget::TabPosition tabPosition;
#endif
    bool ignoreGeometryChange;
    bool ignoreWindowStateChange;
    bool isActivated;
    bool isSubWindowsTiled;
    bool showActiveWindowMaximized;
    bool tileCalledFromResizeEvent;
    bool updatesDisabledByUs;
    bool inViewModeChange;
    int indexToNextWindow;
    int indexToPreviousWindow;
    int indexToHighlighted;
    int indexToLastActiveTab;
    QBasicTimer resizeTimer;
    QBasicTimer tabToPreviousTimer;

    // Slots.
    void _q_deactivateAllWindows(QMdiSubWindow *aboutToActivate = nullptr);
    void _q_processWindowStateChanged(Qt::WindowStates oldState, Qt::WindowStates newState);
    void _q_currentTabChanged(int index);
    void _q_closeTab(int index);
    void _q_moveTab(int from, int to);

    // Functions.
    void appendChild(QMdiSubWindow *child);
    void place(QMdi::Placer *placer, QMdiSubWindow *child);
    void rearrange(QMdi::Rearranger *rearranger);
    void arrangeMinimizedSubWindows();
    void activateWindow(QMdiSubWindow *child);
    void activateCurrentWindow();
    void activateHighlightedWindow();
    void emitWindowActivated(QMdiSubWindow *child);
    void resetActiveWindow(QMdiSubWindow *child = nullptr);
    void updateActiveWindow(int removedIndex, bool activeRemoved);
    void updateScrollBars();
    void internalRaise(QMdiSubWindow *child) const;
    bool scrollBarsEnabled() const;
    bool lastWindowAboutToBeDestroyed() const;
    void setChildActivationEnabled(bool enable = true, bool onlyNextActivationEvent = false) const;
    QRect resizeToMinimumTileSize(const QSize &minSubWindowSize, int subWindowCount);
    void scrollBarPolicyChanged(Qt::Orientation, Qt::ScrollBarPolicy) override; // reimp
    QMdiSubWindow *nextVisibleSubWindow(int increaseFactor, QMdiArea::WindowOrder,
                                        int removed = -1, int fromIndex = -1) const;
    void highlightNextSubWindow(int increaseFactor);
    QList<QMdiSubWindow *> subWindowList(QMdiArea::WindowOrder, bool reversed = false) const;
    void disconnectSubWindow(QObject *subWindow);
    void setViewMode(QMdiArea::ViewMode mode);
#if QT_CONFIG(tabbar)
    void updateTabBarGeometry();
    void refreshTabBar();
#endif

    inline void startResizeTimer()
    {
        Q_Q(QMdiArea);

        using namespace std::chrono_literals;
        resizeTimer.start(200ms, q);
    }

    inline void startTabToPreviousTimer()
    {
        Q_Q(QMdiArea);

        using namespace std::chrono_literals;
        tabToPreviousTimer.start(QApplication::keyboardInputInterval() * 1ms, q);
    }

    inline bool windowStaysOnTop(QMdiSubWindow *subWindow) const
    {
        if (!subWindow)
            return false;
        return subWindow->windowFlags() & Qt::WindowStaysOnTopHint;
    }

    inline bool isExplicitlyDeactivated(QMdiSubWindow *subWindow) const
    {
        if (!subWindow)
            return true;
        return subWindow->d_func()->isExplicitlyDeactivated;
    }

    inline void setActive(QMdiSubWindow *subWindow, bool active = true, bool changeFocus = true) const
    {
        if (subWindow)
            subWindow->d_func()->setActive(active, changeFocus);
    }

#if QT_CONFIG(rubberband)
    void showRubberBandFor(QMdiSubWindow *subWindow);

    inline void hideRubberBand()
    {
        if (rubberBand && rubberBand->isVisible())
            rubberBand->hide();
        indexToHighlighted = -1;
    }
#endif // QT_CONFIG(rubberband)
};

QT_END_NAMESPACE

#endif // QMDIAREA_P_H
