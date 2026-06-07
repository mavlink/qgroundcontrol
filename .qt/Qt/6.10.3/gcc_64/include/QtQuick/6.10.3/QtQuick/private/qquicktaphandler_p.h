// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKTAPHANDLER_H
#define QQUICKTAPHANDLER_H

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

#include <QtCore/qbasictimer.h>
#include <QtCore/qelapsedtimer.h>
#include <QtGui/qevent.h>
#include <QtQuick/qquickitem.h>

#include "qquicksinglepointhandler_p.h"

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QQuickTapHandler : public QQuickSinglePointHandler
{
    Q_OBJECT
    Q_PROPERTY(bool pressed READ isPressed NOTIFY pressedChanged)
    Q_PROPERTY(int tapCount READ tapCount NOTIFY tapCountChanged)
    Q_PROPERTY(qreal timeHeld READ timeHeld NOTIFY timeHeldChanged)
    Q_PROPERTY(qreal longPressThreshold READ longPressThreshold WRITE setLongPressThreshold NOTIFY longPressThresholdChanged RESET resetLongPressThreshold)
    Q_PROPERTY(GesturePolicy gesturePolicy READ gesturePolicy WRITE setGesturePolicy NOTIFY gesturePolicyChanged)
    Q_PROPERTY(QQuickTapHandler::ExclusiveSignals exclusiveSignals READ exclusiveSignals WRITE setExclusiveSignals NOTIFY exclusiveSignalsChanged REVISION(6, 5))

    QML_NAMED_ELEMENT(TapHandler)
    QML_ADDED_IN_VERSION(2, 12)

public:
    enum GesturePolicy {
        DragThreshold,
        WithinBounds,
        ReleaseWithinBounds,
        DragWithinBounds
    };
    Q_ENUM(GesturePolicy)

    enum ExclusiveSignal {
        NotExclusive = 0,
        SingleTap = 1 << 0,
        DoubleTap = 1 << 1
    };
    Q_DECLARE_FLAGS(ExclusiveSignals, ExclusiveSignal)
    Q_FLAG(ExclusiveSignals)

    explicit QQuickTapHandler(QQuickItem *parent = nullptr);

    bool isPressed() const { return m_pressed; }

    int tapCount() const { return m_tapCount; }
    qreal timeHeld() const { return (m_holdTimer.isValid() ? m_holdTimer.elapsed() / 1000.0 : -1.0); }

    qreal longPressThreshold() const;
    void setLongPressThreshold(qreal longPressThreshold);
    void resetLongPressThreshold();

    GesturePolicy gesturePolicy() const { return m_gesturePolicy; }
    void setGesturePolicy(GesturePolicy gesturePolicy);

    QQuickTapHandler::ExclusiveSignals exclusiveSignals() const { return m_exclusiveSignals; }
    void setExclusiveSignals(QQuickTapHandler::ExclusiveSignals newexclusiveSignals);

Q_SIGNALS:
    void pressedChanged();
    void tapCountChanged();
    void timeHeldChanged();
    void longPressThresholdChanged();
    void gesturePolicyChanged();
    Q_REVISION(6, 5) void exclusiveSignalsChanged();
    // the second argument (Qt::MouseButton) was added in 6.2: avoid name clashes with IDs by not naming it for now
    void tapped(QEventPoint eventPoint, Qt::MouseButton /* button */);
    void singleTapped(QEventPoint eventPoint, Qt::MouseButton /* button */);
    void doubleTapped(QEventPoint eventPoint, Qt::MouseButton /* button */);
    void longPressed();

protected:
    void onGrabChanged(QQuickPointerHandler *grabber, QPointingDevice::GrabTransition transition,
                       QPointerEvent *ev, QEventPoint &point) override;
    void timerEvent(QTimerEvent *event) override;
    bool wantsEventPoint(const QPointerEvent *event, const QEventPoint &point) override;
    void handleEventPoint(QPointerEvent *event, QEventPoint &point) override;

private:
    void setPressed(bool press, bool cancel, QPointerEvent *event, QEventPoint &point);
    void connectPreRenderSignal(bool conn = true);
    void updateTimeHeld();

private:
    QPointF m_lastTapPos;
    quint64 m_lastTapTimestamp = 0;
    QElapsedTimer m_holdTimer;
    QBasicTimer m_longPressTimer;
    QBasicTimer m_doubleTapTimer;
    QEventPoint m_singleTapReleasedPoint;
    QMetaObject::Connection m_preRenderSignalConnection;
    Qt::MouseButton m_singleTapReleasedButton;
    int m_tapCount = 0;
    int m_longPressThreshold = -1;
    GesturePolicy m_gesturePolicy = GesturePolicy::DragThreshold;
    ExclusiveSignals m_exclusiveSignals = NotExclusive;
    bool m_pressed = false;
    bool m_longPressed = false;

    static quint64 m_multiTapInterval;
    static int m_mouseMultiClickDistanceSquared;
    static int m_touchMultiTapDistanceSquared;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QQuickTapHandler::ExclusiveSignals)

QT_END_NAMESPACE

#endif // QQUICKTAPHANDLER_H
