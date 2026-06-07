// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKDELIVERYAGENT_P_P_H
#define QQUICKDELIVERYAGENT_P_P_H

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

#include <QtQuick/private/qquickdeliveryagent_p.h>
#include <QtGui/qevent.h>
#include <QtCore/qstack.h>
#include <QtCore/qxpfunctional.h>

#include <private/qevent_p.h>
#include <private/qpointingdevice_p.h>

#include <QtCore/qpointer.h>
#include <private/qobject_p.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QQuickDragGrabber;
class QQuickItem;
class QQuickPointerHandler;
class QQuickWindow;

/*! \internal
    Extra device-specific data to be stored in QInputDevicePrivate::qqExtra
*/
struct QQuickPointingDeviceExtra {
    // used in QQuickPointerHandlerPrivate::deviceDeliveryTargets
    QVector<QObject *> deliveryTargets;
};

class Q_QUICK_EXPORT QQuickDeliveryAgentPrivate : public QObjectPrivate
{
public:
    Q_DECLARE_PUBLIC(QQuickDeliveryAgent)
    QQuickDeliveryAgentPrivate(QQuickItem *root);
    ~QQuickDeliveryAgentPrivate();

    QQuickItem *rootItem = nullptr;

    QQuickItem *activeFocusItem = nullptr;

    void deliverKeyEvent(QKeyEvent *e);

    enum FocusOption {
        DontChangeFocusProperty = 0x01,
        DontChangeSubFocusItem  = 0x02
    };
    Q_DECLARE_FLAGS(FocusOptions, FocusOption)

    void setFocusInScope(QQuickItem *scope, QQuickItem *item, Qt::FocusReason reason, FocusOptions = { });
    void clearFocusInScope(QQuickItem *scope, QQuickItem *item, Qt::FocusReason reason, FocusOptions = { });
    static void notifyFocusChangesRecur(QQuickItem **item, int remaining, Qt::FocusReason reason);
    void clearFocusObject();
    void updateFocusItemTransform();

    QQuickItem *focusTargetItem() const;

    // Keeps track of the item currently receiving mouse events
#if QT_CONFIG(quick_draganddrop)
    QQuickDragGrabber *dragGrabber = nullptr;
#endif
    QQuickItem *lastUngrabbed = nullptr;
    QStack<QPointerEvent *> eventsInDelivery;
    QFlatMap<QPointer<QQuickItem>, uint> hoverItems;
    QVector<QQuickItem *> hasFiltered; // during event delivery to a single receiver, the filtering parents for which childMouseEventFilter was already called
    QVector<QQuickItem *> skipDelivery; // during delivery of one event to all receivers, Items to which we know delivery is no longer necessary

    std::unique_ptr<QMutableTouchEvent> delayedTouch;
    QList<const QPointingDevice *> knownPointingDevices;

    uint currentHoverId = 0;
#if QT_CONFIG(wheelevent)
    uint lastWheelEventAccepted = 0;
#endif
    uchar compressedTouchCount = 0;
    bool allowChildEventFiltering = true;
    bool frameSynchronousHoverEnabled = true;
    bool hoveredLeafItemFound = false;

    bool isSubsceneAgent = false;
    static bool subsceneAgentsExist;
    // QQuickDeliveryAgent::event() sets this to the one that's currently (trying to) handle the event
    static QQuickDeliveryAgent *currentEventDeliveryAgent;
    static QQuickDeliveryAgent *currentOrItemDeliveryAgent(const QQuickItem *item);

    Qt::FocusReason lastFocusReason = Qt::OtherFocusReason;
    int pointerEventRecursionGuard = 0;

    int touchMouseId = -1; // only for obsolete stuff like QQuickItem::grabMouse()
    // TODO get rid of these
    const QPointingDevice *touchMouseDevice = nullptr;
    ulong touchMousePressTimestamp = 0;
    QPoint touchMousePressPos;      // in screen coordinates

    QQuickDeliveryAgent::Transform *sceneTransform = nullptr;

    bool isDeliveringTouchAsMouse() const { return touchMouseId != -1 && touchMouseDevice; }
    void cancelTouchMouseSynthesis();

    bool checkIfDoubleTapped(ulong newPressEventTimestamp, const QPoint &newPressPos);
    void resetIfDoubleTapPrevented(const QEventPoint &pressedPoint);
    QPointingDevicePrivate::EventPointData *mousePointData();
    QPointerEvent *eventInDelivery() const;

    // Mouse positions are saved in widget coordinates
    QPointF lastMousePosition;
    bool deliverTouchAsMouse(QQuickItem *item, QTouchEvent *pointerEvent);
    static void translateTouchEvent(QTouchEvent *touchEvent);
    void removeGrabber(QQuickItem *grabber, bool mouse = true, bool touch = true, bool cancel = false);
    void clearGrabbers(QPointerEvent *pointerEvent);
    void onGrabChanged(QObject *grabber, QPointingDevice::GrabTransition transition, const QPointerEvent *event, const QEventPoint &point);
    static QPointerEvent *clonePointerEvent(QPointerEvent *event, std::optional<QPointF> transformedLocalPos = std::nullopt);
    void deliverToPassiveGrabbers(const QVector<QPointer<QObject> > &passiveGrabbers, QPointerEvent *pointerEvent);
    bool sendFilteredMouseEvent(QEvent *event, QQuickItem *receiver, QQuickItem *filteringParent);
    bool sendFilteredPointerEvent(QPointerEvent *event, QQuickItem *receiver, QQuickItem *filteringParent = nullptr);
    bool sendFilteredPointerEventImpl(QPointerEvent *event, QQuickItem *receiver, QQuickItem *filteringParent);
    bool deliverSinglePointEventUntilAccepted(QPointerEvent *);

    // entry point of events to the window
    void handleTouchEvent(QTouchEvent *);
    void handleMouseEvent(QMouseEvent *);
    bool compressTouchEvent(QTouchEvent *);
    void flushFrameSynchronousEvents(QQuickWindow *win);
    void deliverDelayedTouchEvent();
    void handleWindowDeactivate(QQuickWindow *win);
    void handleWindowHidden(QQuickWindow *win);

    // utility functions that used to be in QQuickPointerEvent et al.
    bool allUpdatedPointsAccepted(const QPointerEvent *ev);
    static void localizePointerEvent(QPointerEvent *ev, const QQuickItem *dest);
    QList<QObject *> exclusiveGrabbers(QPointerEvent *ev);
    static bool anyPointGrabbed(const QPointerEvent *ev);
    static bool allPointsGrabbed(const QPointerEvent *ev);
    static bool isMouseEvent(const QPointerEvent *ev);
    static bool isMouseOrWheelEvent(const QPointerEvent *ev);
    static bool isHoverEvent(const QPointerEvent *ev);
    static bool isHoveringMoveEvent(const QPointerEvent *ev);
    static bool isTouchEvent(const QPointerEvent *ev);
    static bool isTabletEvent(const QPointerEvent *ev);
    static bool isEventFromMouseOrTouchpad(const QPointerEvent *ev);
    static bool isSynthMouse(const QPointerEvent *ev);
    static bool isWithinDoubleClickInterval(ulong timeInterval);
    static bool isWithinDoubleTapDistance(const QPoint &distanceBetweenPresses);
    static bool isSinglePointDevice(const QInputDevice *dev);
    static QQuickPointingDeviceExtra *deviceExtra(const QInputDevice *device);

    // delivery of pointer events:
    void touchToMouseEvent(QEvent::Type type, const QEventPoint &p, const QTouchEvent *touchEvent, QMutableSinglePointEvent *mouseEvent);
    void ensureDeviceConnected(const QPointingDevice *dev);
    void deliverPointerEvent(QPointerEvent *);
    bool deliverTouchCancelEvent(QTouchEvent *);
    bool deliverPressOrReleaseEvent(QPointerEvent *, bool handlersOnly = false);
    void deliverUpdatedPoints(QPointerEvent *event);
    void deliverMatchingPointsToItem(QQuickItem *item, bool isGrabber, QPointerEvent *pointerEvent, bool handlersOnly = false);

    QVector<QQuickItem *> eventTargets(QQuickItem *, const QEvent *event, QPointF scenePos, qxp::function_ref<std::optional<bool> (QQuickItem *, const QEvent *)> predicate) const;
    QVector<QQuickItem *> pointerTargets(QQuickItem *, const QPointerEvent *event, const QEventPoint &point,
                                         bool checkMouseButtons, bool checkAcceptsTouch) const;
    QVector<QQuickItem *> mergePointerTargets(const QVector<QQuickItem *> &list1, const QVector<QQuickItem *> &list2) const;

    // hover delivery
    enum class HoverChange : uint8_t {
        Clear,
        Set,
    };
    bool deliverHoverEvent(const QPointF &scenePos, const QPointF &lastScenePos, Qt::KeyboardModifiers modifiers, ulong timestamp);
    bool deliverHoverEventRecursive(QQuickItem *, const QPointF &scenePos, const QPointF &lastScenePos, Qt::KeyboardModifiers modifiers, ulong timestamp);
    bool deliverHoverEventToItem(QQuickItem *item, const QPointF &scenePos, const QPointF &lastScenePos, Qt::KeyboardModifiers modifiers, ulong timestamp,
                                 HoverChange hoverChange);
    bool sendHoverEvent(QEvent::Type, QQuickItem *, const QPointF &scenePos, const QPointF &lastScenePos,
                        Qt::KeyboardModifiers modifiers, ulong timestamp);
    bool clearHover(ulong timestamp = 0);

#if QT_CONFIG(quick_draganddrop)
    void deliverDragEvent(QQuickDragGrabber *, QEvent *);
    bool deliverDragEvent(QQuickDragGrabber *, QQuickItem *, QDragMoveEvent *,
                          QVarLengthArray<QQuickItem *, 64> *currentGrabItems = nullptr,
                          QObject *formerTarget = nullptr);
#endif

    static bool dragOverThreshold(qreal d, Qt::Axis axis, QMouseEvent *event, int startDragThreshold = -1);

    static bool dragOverThreshold(qreal d, Qt::Axis axis, const QEventPoint &tp, int startDragThreshold = -1);

    static bool dragOverThreshold(QVector2D delta);

    // context menu events
    QVector<QQuickItem *> contextMenuTargets(QQuickItem *item, const QContextMenuEvent *event) const;
    void deliverContextMenuEvent(QContextMenuEvent *event);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QQuickDeliveryAgentPrivate::FocusOptions)

QT_END_NAMESPACE

#endif // QQUICKDELIVERYAGENT_P_P_H
