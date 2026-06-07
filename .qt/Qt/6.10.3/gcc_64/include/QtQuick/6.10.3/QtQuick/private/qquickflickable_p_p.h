// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKFLICKABLE_P_P_H
#define QQUICKFLICKABLE_P_P_H

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

#include "qquickflickable_p.h"
#include "qquickitem_p.h"
#include "qquickitemchangelistener_p.h"

#include <QtQml/qqml.h>
#include <QtCore/qdatetime.h>
#include "qplatformdefs.h"

#include <private/qquicktimeline_p_p.h>
#include <private/qquickanimation_p_p.h>
#include <private/qquicktransitionmanager_p_p.h>
#include <private/qpodvector_p.h>

QT_BEGIN_NAMESPACE

class QQuickFlickableVisibleArea;
class QQuickTransition;
class QQuickFlickableReboundTransition;

class Q_QUICK_EXPORT QQuickFlickablePrivate : public QQuickItemPrivate,
                                              public QSafeQuickItemChangeListener<QQuickFlickablePrivate>
{
    Q_DECLARE_PUBLIC(QQuickFlickable)

public:
    static inline QQuickFlickablePrivate *get(QQuickFlickable *o) { return o->d_func(); }

    QQuickFlickablePrivate();
    void init();

    struct Velocity : public QQuickTimeLineValue
    {
        Velocity(QQuickFlickablePrivate *p)
            : parent(p) {}
        void setValue(qreal v) override {
            if (v != value()) {
                QQuickTimeLineValue::setValue(v);
                parent->updateVelocity();
            }
        }
        QQuickFlickablePrivate *parent;
    };

    enum MovementReason { Other, SetIndex, Mouse };

    struct AxisData {
        AxisData(QQuickFlickablePrivate *fp, void (QQuickFlickablePrivate::*func)(qreal))
            : move(fp, func)
            , smoothVelocity(fp), atEnd(false), atBeginning(true)
            , transitionToSet(false)
            , fixingUp(false), inOvershoot(false), inRebound(false), moving(false), flicking(false)
            , flickingWhenDragBegan(false), dragging(false), extentsChanged(false)
            , explicitValue(false), contentPositionChangedExternallyDuringDrag(false)
            , minExtentDirty(true), maxExtentDirty(true)
        {}

        ~AxisData();

        void reset() {
            velocityBuffer.clear();
            dragStartOffset = 0;
            fixingUp = false;
            inOvershoot = false;
            contentPositionChangedExternallyDuringDrag = false;
        }

        void markExtentsDirty() {
            minExtentDirty = true;
            maxExtentDirty = true;
            extentsChanged = true;
        }

        void resetTransitionTo() {
            transitionTo = 0;
            transitionToSet = false;
        }

        void addVelocitySample(qreal v, qreal maxVelocity);
        void updateVelocity();

        QQuickTimeLineValueProxy<QQuickFlickablePrivate> move;
        QQuickFlickableReboundTransition *transitionToBounds = nullptr;
        qreal viewSize = -1;
        qreal pressPos = 0;
        qreal lastPos = 0;
        qreal dragStartOffset = 0;
        qreal dragMinBound = 0;
        qreal dragMaxBound = 0;
        qreal previousDragDelta = 0;
        qreal velocity = 0;
        qreal flickTarget = 0;
        qreal startMargin = 0;
        qreal endMargin = 0;
        qreal origin = 0;
        qreal overshoot = 0;
        qreal transitionTo = 0;
        qreal continuousFlickVelocity = 0;
        QElapsedTimer velocityTime;
        int vTime = 0;
        QQuickFlickablePrivate::Velocity smoothVelocity;
        QPODVector<qreal,10> velocityBuffer;
        // bitfield
        uint atEnd : 1;
        uint atBeginning : 1;
        uint transitionToSet : 1;
        uint fixingUp : 1;
        uint inOvershoot : 1;
        uint inRebound : 1;
        uint moving : 1;
        uint flicking : 1;
        uint flickingWhenDragBegan : 1;
        uint dragging : 1;
        uint extentsChanged : 1;
        uint explicitValue : 1;
        uint contentPositionChangedExternallyDuringDrag : 1;
        // mutable portion of bitfield
        mutable uint minExtentDirty : 1;
        mutable uint maxExtentDirty : 1;
        // end bitfield
    };

    bool flickX(QEvent::Type eventType, qreal velocity);
    bool flickY(QEvent::Type eventType, qreal velocity);
    virtual bool flick(AxisData &data, qreal minExtent, qreal maxExtent, qreal vSize,
                       QQuickTimeLineCallback::Callback fixupCallback,
                       QEvent::Type eventType, qreal velocity);
    void flickingStarted(bool flickingH, bool flickingV);

    void fixupX();
    void fixupY();
    virtual void fixup(AxisData &data, qreal minExtent, qreal maxExtent);
    void adjustContentPos(AxisData &data, qreal toPos);
    void resetTimeline(AxisData &data);
    void clearTimeline();

    void updateBeginningEnd();

    bool isInnermostPressDelay(QQuickItem *item) const;
    void captureDelayedPress(QQuickItem *item, QPointerEvent *event);
    void clearDelayedPress();
    void replayDelayedPress();

    void setViewportX(qreal x);
    void setViewportY(qreal y);

    qreal overShootDistance(qreal velocity) const;

    void itemGeometryChanged(QQuickItem *, QQuickGeometryChange, const QRectF &) override;

    void draggingStarting();
    void draggingEnding();

    bool isViewMoving() const;

    void cancelInteraction();

    void addPointerHandler(QQuickPointerHandler *h) override;

    virtual bool wantsPointerEvent(const QPointerEvent *) { return true; }

public:
    QQuickItem *contentItem;

    AxisData hData;
    AxisData vData;

    MovementReason moveReason = Other;

    QQuickTimeLine timeline;
    bool hMoved : 1;
    bool vMoved : 1;
    bool stealMouse : 1;
    bool pressed : 1;
    bool scrollingPhase : 1;
    bool interactive : 1;
    bool calcVelocity : 1;
    bool pixelAligned : 1;
    bool syncDrag : 1;
    Qt::MouseButtons acceptedButtons = Qt::LeftButton;
    QElapsedTimer timer;
    qint64 lastPosTime;
    qint64 lastPressTime;
    QPointF lastPos;
    QPointF pressPos;
    QVector2D accumulatedWheelPixelDelta;
    qreal deceleration;
    qreal wheelDeceleration;
    qreal maxVelocity;
    QPointerEvent *delayedPressEvent;
    QBasicTimer delayedPressTimer;
    int pressDelay;
    int fixupDuration;
    qreal flickBoost;
    qreal initialWheelFlickDistance;

    enum FixupMode { Normal, Immediate, ExtentChanged };
    FixupMode fixupMode;

    static void fixupY_callback(void *);
    static void fixupX_callback(void *);

    void updateVelocity();
    int vTime;
    QQuickTimeLine velocityTimeline;
    QQuickFlickableVisibleArea *visibleArea;
    QQuickFlickable::FlickableDirection flickableDirection;
    QQuickFlickable::BoundsBehavior boundsBehavior;
    QQuickFlickable::BoundsMovement boundsMovement;
    QQuickTransition *rebound;

    void viewportAxisMoved(AxisData &data, qreal minExtent, qreal maxExtent,
                       QQuickTimeLineCallback::Callback fixupCallback);

    void handlePressEvent(QPointerEvent *);
    void handleMoveEvent(QPointerEvent *);
    void handleReleaseEvent(QPointerEvent *);
    bool buttonsAccepted(const QSinglePointEvent *event);

    void maybeBeginDrag(qint64 currentTimestamp, const QPointF &pressPosn,
                        Qt::MouseButtons buttons = Qt::NoButton);
    void drag(qint64 currentTimestamp, QEvent::Type eventType, const QPointF &localPos,
              const QVector2D &deltas, bool overThreshold, bool momentum,
              bool velocitySensitiveOverBounds, const QVector2D &velocity);

    QVector2D firstPointLocalVelocity(QPointerEvent *event);
    qint64 computeCurrentTime(QInputEvent *event) const;

    // flickableData property
    static void data_append(QQmlListProperty<QObject> *, QObject *);
    static qsizetype data_count(QQmlListProperty<QObject> *);
    static QObject *data_at(QQmlListProperty<QObject> *, qsizetype);
    static void data_clear(QQmlListProperty<QObject> *);
};

class Q_QUICK_EXPORT QQuickFlickableVisibleArea : public QObject
{
    Q_OBJECT

    Q_PROPERTY(qreal xPosition READ xPosition NOTIFY xPositionChanged FINAL)
    Q_PROPERTY(qreal yPosition READ yPosition NOTIFY yPositionChanged FINAL)
    Q_PROPERTY(qreal widthRatio READ widthRatio NOTIFY widthRatioChanged FINAL)
    Q_PROPERTY(qreal heightRatio READ heightRatio NOTIFY heightRatioChanged FINAL)
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickFlickableVisibleArea(QQuickFlickable *parent=nullptr);

    qreal xPosition() const;
    qreal widthRatio() const;
    qreal yPosition() const;
    qreal heightRatio() const;

    void updateVisible();

Q_SIGNALS:
    void xPositionChanged(qreal xPosition);
    void yPositionChanged(qreal yPosition);
    void widthRatioChanged(qreal widthRatio);
    void heightRatioChanged(qreal heightRatio);

private:
    QQuickFlickable *flickable;
    qreal m_xPosition;
    qreal m_widthRatio;
    qreal m_yPosition;
    qreal m_heightRatio;
};

QT_END_NAMESPACE

#endif // QQUICKFLICKABLE_P_P_H
