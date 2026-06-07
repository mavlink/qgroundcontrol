// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSCROLLER_P_H
#define QSCROLLER_P_H

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
#include <QObject>
#include <QPointer>
#include <QQueue>
#include <QSet>
#include <QEasingCurve>
#include <QElapsedTimer>
#include <QSizeF>
#include <QPointF>
#include <QRectF>
#include <qscroller.h>
#include <qscrollerproperties.h>
#include <private/qscrollerproperties_p.h>
#if QT_CONFIG(animation)
#include <QAbstractAnimation>
#endif

QT_BEGIN_NAMESPACE

#ifndef QT_NO_GESTURES
class QFlickGestureRecognizer;
#endif

#if QT_CONFIG(animation)
class QScrollTimer;
#endif
class QScrollerPrivate : public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(QScroller)

public:
    QScrollerPrivate(QScroller *q, QObject *target);
    void init();

    void sendEvent(QObject *o, QEvent *e);

    void setState(QScroller::State s);

    enum ScrollType {
        ScrollTypeFlick = 0,
        ScrollTypeScrollTo,
        ScrollTypeOvershoot
    };

    struct ScrollSegment {
        qint64 startTime;
        qint64 deltaTime;
        qreal startPos;
        qreal deltaPos;
        QEasingCurve curve;
        qreal stopProgress; // whatever is..
        qreal stopPos;      // ..reached first
        ScrollType type;
    };

    bool pressWhileInactive(const QPointF &position, qint64 timestamp);
    bool moveWhilePressed(const QPointF &position, qint64 timestamp);
    bool releaseWhilePressed(const QPointF &position, qint64 timestamp);
    bool moveWhileDragging(const QPointF &position, qint64 timestamp);
    bool releaseWhileDragging(const QPointF &position, qint64 timestamp);
    bool pressWhileScrolling(const QPointF &position, qint64 timestamp);

    void timerTick();
    void timerEventWhileDragging();
    void timerEventWhileScrolling();

    bool prepareScrolling(const QPointF &position);
    void handleDrag(const QPointF &position, qint64 timestamp);

    QPointF dpi() const;
    void setDpi(const QPointF &dpi);
    void setDpiFromWidget(QWidget *widget);

    void updateVelocity(const QPointF &deltaPixelRaw, qint64 deltaTime);
    void pushSegment(ScrollType type, qreal deltaTime, qreal stopProgress, qreal startPos, qreal deltaPos, qreal stopPos, QEasingCurve::Type curve, Qt::Orientation orientation);
    void recalcScrollingSegments(bool forceRecalc = false);
    qreal scrollingSegmentsEndPos(Qt::Orientation orientation) const;
    bool scrollingSegmentsValid(Qt::Orientation orientation) const;
    void createScrollToSegments(qreal v, qreal deltaTime, qreal endPos, Qt::Orientation orientation, ScrollType type);
    void createScrollingSegments(qreal v, qreal startPos,
                                 qreal deltaTime, qreal deltaPos,
                                 Qt::Orientation orientation);
    void createScrollingSegments(const QPointF &v, const QPointF &startPos, const QPointF &ppm);

    void setContentPositionHelperDragging(const QPointF &deltaPos);
    void setContentPositionHelperScrolling();

    qreal nextSnapPos(qreal p, int dir, Qt::Orientation orientation) const;
    static qreal nextSegmentPosition(QQueue<ScrollSegment> &segments, qint64 now, qreal oldPos);

    inline int frameRateSkip() const { return properties.d.data()->frameRate; }

    static const char *stateName(QScroller::State state);
    static const char *inputName(QScroller::Input input);

public slots:
    void targetDestroyed();

public:
    // non static
    QObject *target;
    QScrollerProperties properties;
#ifndef QT_NO_GESTURES
    QFlickGestureRecognizer *recognizer;
    Qt::GestureType recognizerType;
#endif

    // scroller state:

    // QPointer<QObject> scrollTarget;
    QSizeF viewportSize;
    QRectF contentPosRange;
    QPointF contentPosition;
    QPointF overshootPosition; // the number of pixels we are overshooting (before overshootDragResistanceFactor)

    // state

    bool enabled;
    QScroller::State state;
    bool firstScroll; // true if we haven't already send a scroll event

    QPointF oldVelocity; // the release velocity of the last drag

    QPointF pressPosition;
    QPointF lastPosition;
    qint64  pressTimestamp;
    qint64  lastTimestamp;

    QPointF dragDistance; // the distance we should move during the next drag timer event

    QQueue<ScrollSegment> xSegments;
    QQueue<ScrollSegment> ySegments;

    // snap positions
    QList<qreal> snapPositionsX;
    qreal snapFirstX;
    qreal snapIntervalX;
    QList<qreal> snapPositionsY;
    qreal snapFirstY;
    qreal snapIntervalY;

    QPointF pixelPerMeter;

    QElapsedTimer monotonicTimer;

    QPointF releaseVelocity; // the starting velocity of the scrolling state
#if QT_CONFIG(animation)
    QScrollTimer *scrollTimer;
#endif

    QScroller *q_ptr;
};
template <>
class QTypeInfo<QScrollerPrivate::ScrollSegment>
    : public QTypeInfoMerger<QScrollerPrivate::ScrollSegment, QEasingCurve> {};


QT_END_NAMESPACE

#endif // QSCROLLER_P_H

