// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKPOPUP_P_P_H
#define QQUICKPOPUP_P_P_H

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

#include <QtQuickTemplates2/private/qquickpopup_p.h>
#include <QtQuickTemplates2/private/qquickcontrol_p.h>
#include <QtQuickTemplates2/private/qquicktheme_p.h>

#include <QtCore/private/qobject_p.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/private/qquickitemchangelistener_p.h>
#include <QtQuick/private/qquicktransitionmanager_p_p.h>
#include <QtQuick/private/qquickitem_p.h>

#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

class QQuickTransition;
class QQuickTransitionManager;
class QQuickPopup;
class QQuickPopupAnchors;
class QQuickPopupItem;
class QQuickPopupWindow;
class QQuickPopupPrivate;
class QQuickPopupPositioner;

class Q_QUICKTEMPLATES2_EXPORT QQuickPopupTransitionManager : public QQuickTransitionManager
{
public:
    QQuickPopupTransitionManager(QQuickPopupPrivate *popup);

    void transitionEnter();
    void transitionExit();

protected:
    void finished() override;

private:
    QQuickPopupPrivate *popup = nullptr;
};

class Q_QUICKTEMPLATES2_EXPORT QQuickPopupPrivate
    : public QObjectPrivate
    , public QSafeQuickItemChangeListener<QQuickPopupPrivate>
    , public QQuickPaletteProviderPrivateBase<QQuickPopup, QQuickPopupPrivate>
{
public:
    Q_DECLARE_PUBLIC(QQuickPopup)

    QQuickPopupPrivate();

    static QQuickPopupPrivate *get(QQuickPopup *popup)
    {
        return popup->d_func();
    }

    QQmlListProperty<QObject> contentData();
    QQmlListProperty<QQuickItem> contentChildren();

    void init();
    void closeOrReject();
    bool tryClose(const QPointF &pos, QQuickPopup::ClosePolicy flags);

    bool contains(const QPointF &scenePos) const;

#if QT_CONFIG(quicktemplates2_multitouch)
    virtual bool acceptTouch(const QTouchEvent::TouchPoint &point);
#endif
    virtual bool blockInput(QQuickItem *item, const QPointF &point) const;

    virtual bool handlePress(QQuickItem* item, const QPointF &point, ulong timestamp);
    virtual bool handleMove(QQuickItem* item, const QPointF &point, ulong timestamp);
    virtual bool handleRelease(QQuickItem* item, const QPointF &point, ulong timestamp);
    virtual bool handleReleaseWithoutGrab(const QEventPoint &) { return false; }
    virtual void handleUngrab();

    bool handleMouseEvent(QQuickItem *item, QMouseEvent *event);
    bool handleHoverEvent(QQuickItem *item, QHoverEvent *event);
#if QT_CONFIG(quicktemplates2_multitouch)
    bool handleTouchEvent(QQuickItem *item, QTouchEvent *event);
#endif

    QMarginsF windowInsets() const;
    QPointF windowInsetsTopLeft() const;
    void setEffectivePosFromWindowPos(const QPointF &windowPos);
    void reposition();

    bool usePopupWindow() const;
    void adjustPopupItemParentAndWindow();
    void createOverlay();
    QQuickItem *createDimmer(QQmlComponent *component, QQuickPopup *popup, QQuickItem *parent) const;
    void destroyDimmer();
    void toggleOverlay();
    void updateContentPalettes(const QPalette& parentPalette);

    virtual QQuickPopup::PopupType resolvedPopupType() const;

    virtual void showDimmer();
    virtual void hideDimmer();
    virtual void resizeDimmer();

    virtual bool prepareEnterTransition();
    virtual bool prepareExitTransition();
    virtual void finalizeEnterTransition();
    virtual void finalizeExitTransition();

    virtual void opened();

    Qt::WindowFlags popupWindowFlags() const;
    void setPopupWindowFlags(Qt::WindowFlags flags);

    QMarginsF getMargins() const;

    void setTopMargin(qreal value, bool reset = false);
    void setLeftMargin(qreal value, bool reset = false);
    void setRightMargin(qreal value, bool reset = false);
    void setBottomMargin(qreal value, bool reset = false);

    QQuickPopupAnchors *getAnchors();
    virtual QQuickPopupPositioner *getPositioner();

    void setWindow(QQuickWindow *window);
    void itemDestroyed(QQuickItem *item) override;

    QPalette defaultPalette() const override;

    void updateChildrenPalettes(const QPalette &parentPalette) override;

    enum TransitionState {
        NoTransition, EnterTransition, ExitTransition
    };

    static const QQuickPopup::ClosePolicy DefaultClosePolicy;

    bool focus = false;
    bool modal = false;
    bool dim = false;
    bool hasDim = false;
    bool visible = false;
    bool complete = true;
    bool positioning = false;
    bool hasWidth = false;
    bool hasHeight = false;
    bool hasTopMargin = false;
    bool hasLeftMargin = false;
    bool hasRightMargin = false;
    bool hasBottomMargin = false;
    bool hasZ = false;
    bool allowVerticalFlip = false;
    bool allowHorizontalFlip = false;
    bool allowVerticalMove = true;
    bool allowHorizontalMove = true;
    bool allowVerticalResize = true;
    bool allowHorizontalResize = true;
    bool hadActiveFocusBeforeExitTransition = false;
    bool interactive = true;
    bool hasClosePolicy = false;
    bool outsidePressed = false;
    bool outsideParentPressed = false;
    bool inDestructor = false;
    bool relaxEdgeConstraint = false;
    bool popupWindowDirty = false;
    QPointer<QQuickItem> lastActiveFocusItem;
    int touchId = -1;
    qreal x = 0;
    qreal y = 0;
    QPointF effectivePos;
    qreal margins = -1;
    qreal topMargin = 0;
    qreal leftMargin = 0;
    qreal rightMargin = 0;
    qreal bottomMargin = 0;
    QPointF pressPoint;
    TransitionState transitionState = NoTransition;
    QQuickPopup::ClosePolicy closePolicy = DefaultClosePolicy;
    QQuickItem *parentItem = nullptr;
    QQuickItem *dimmer = nullptr;
    QPointer<QQuickWindow> window;
    QQuickTransition *enter = nullptr;
    QQuickTransition *exit = nullptr;
    QQuickPopupItem *popupItem = nullptr;
    QQuickPopupWindow *popupWindow = nullptr;
    QQuickPopupPositioner *positioner = nullptr;
    QList<QQuickStateAction> enterActions;
    QList<QQuickStateAction> exitActions;
    QQuickPopupTransitionManager transitionManager;
    QQuickPopupAnchors *anchors = nullptr;
    qreal explicitDimmerOpacity = 0;
    qreal prevOpacity = 0;
    qreal prevScale = 0;
    QString title;
    QQuickPopup::PopupType popupType = QQuickPopup::Item;
    Qt::WindowModality popupWndModality = Qt::NonModal;
    Qt::WindowFlags windowFlags = Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint;

    friend class QQuickPopupTransitionManager;
};

QT_END_NAMESPACE

#endif // QQUICKPOPUP_P_P_H
