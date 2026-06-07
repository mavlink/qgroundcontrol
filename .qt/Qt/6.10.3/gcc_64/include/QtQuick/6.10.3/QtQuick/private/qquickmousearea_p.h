// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKMOUSEAREA_P_H
#define QQUICKMOUSEAREA_P_H

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

#include "qquickitem.h"
#include <private/qtquickglobal_p.h>
#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE

class QQuickMouseEvent;
class QQuickDrag;
class QQuickMouseAreaPrivate;
class QQuickWheelEvent;

class Q_QUICK_EXPORT QQuickMouseArea : public QQuickItem
{
    Q_OBJECT

    Q_PROPERTY(qreal mouseX READ mouseX NOTIFY mouseXChanged)
    Q_PROPERTY(qreal mouseY READ mouseY NOTIFY mouseYChanged)
    Q_PROPERTY(bool containsMouse READ hovered NOTIFY hoveredChanged)
    Q_PROPERTY(bool pressed READ isPressed NOTIFY pressedChanged)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool scrollGestureEnabled READ isScrollGestureEnabled WRITE setScrollGestureEnabled NOTIFY scrollGestureEnabledChanged REVISION(2, 5))
    Q_PROPERTY(Qt::MouseButtons pressedButtons READ pressedButtons NOTIFY pressedButtonsChanged)
    Q_PROPERTY(Qt::MouseButtons acceptedButtons READ acceptedButtons WRITE setAcceptedButtons NOTIFY acceptedButtonsChanged)
    Q_PROPERTY(bool hoverEnabled READ hoverEnabled WRITE setHoverEnabled NOTIFY hoverEnabledChanged)
#if QT_CONFIG(quick_draganddrop)
    Q_PROPERTY(QQuickDrag *drag READ drag CONSTANT) //### add flicking to QQuickDrag or add a QQuickFlick ???
#endif
    Q_PROPERTY(bool preventStealing READ preventStealing WRITE setPreventStealing NOTIFY preventStealingChanged)
    Q_PROPERTY(bool propagateComposedEvents READ propagateComposedEvents WRITE setPropagateComposedEvents NOTIFY propagateComposedEventsChanged)
#if QT_CONFIG(cursor)
    Q_PROPERTY(Qt::CursorShape cursorShape READ cursorShape WRITE setCursorShape RESET unsetCursor NOTIFY cursorShapeChanged)
#endif
    Q_PROPERTY(bool containsPress READ containsPress NOTIFY containsPressChanged REVISION(2, 4))
    Q_PROPERTY(int pressAndHoldInterval READ pressAndHoldInterval WRITE setPressAndHoldInterval NOTIFY pressAndHoldIntervalChanged RESET resetPressAndHoldInterval REVISION(2, 9))
    QML_NAMED_ELEMENT(MouseArea)
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickMouseArea(QQuickItem *parent=nullptr);
    ~QQuickMouseArea();

    qreal mouseX() const;
    qreal mouseY() const;

    bool isEnabled() const;
    void setEnabled(bool);

    bool isScrollGestureEnabled() const;
    void setScrollGestureEnabled(bool);

    bool hovered() const;
    bool isPressed() const;
    bool containsPress() const;

    Qt::MouseButtons pressedButtons() const;

    Qt::MouseButtons acceptedButtons() const;
    void setAcceptedButtons(Qt::MouseButtons buttons);

    bool hoverEnabled() const;
    void setHoverEnabled(bool h);

#if QT_CONFIG(quick_draganddrop)
    QQuickDrag *drag();
#endif

    bool preventStealing() const;
    void setPreventStealing(bool prevent);

    bool propagateComposedEvents() const;
    void setPropagateComposedEvents(bool propagate);

#if QT_CONFIG(cursor)
    Qt::CursorShape cursorShape() const;
    void setCursorShape(Qt::CursorShape shape);
#endif

    int pressAndHoldInterval() const;
    void setPressAndHoldInterval(int interval);
    void resetPressAndHoldInterval();

Q_SIGNALS:
    void hoveredChanged();
    void pressedChanged();
    void enabledChanged();
    Q_REVISION(2, 5) void scrollGestureEnabledChanged();
    void pressedButtonsChanged();
    void acceptedButtonsChanged();
    void hoverEnabledChanged();
#if QT_CONFIG(cursor)
    void cursorShapeChanged();
#endif
    void positionChanged(QQuickMouseEvent *mouse);
    void mouseXChanged(QQuickMouseEvent *mouse);
    void mouseYChanged(QQuickMouseEvent *mouse);
    void preventStealingChanged();
    void propagateComposedEventsChanged();

    void pressed(QQuickMouseEvent *mouse);
    void pressAndHold(QQuickMouseEvent *mouse);
    void released(QQuickMouseEvent *mouse);
    void clicked(QQuickMouseEvent *mouse);
    void doubleClicked(QQuickMouseEvent *mouse);
#if QT_CONFIG(wheelevent)
    void wheel(QQuickWheelEvent *wheel);
#endif
    void entered();
    void exited();
    void canceled();
    Q_REVISION(2, 4) void containsPressChanged();
    Q_REVISION(2, 9) void pressAndHoldIntervalChanged();

protected:
    void setHovered(bool);
    bool setPressed(Qt::MouseButton button, bool p, Qt::MouseEventSource source);
    bool sendMouseEvent(QMouseEvent *event);

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseUngrabEvent() override;
    void touchUngrabEvent() override;
    void hoverEnterEvent(QHoverEvent *event) override;
    void hoverMoveEvent(QHoverEvent *event) override;
    void hoverLeaveEvent(QHoverEvent *event) override;
#if QT_CONFIG(wheelevent)
    void wheelEvent(QWheelEvent *event) override;
#endif
    bool childMouseEventFilter(QQuickItem *i, QEvent *e) override;
    void timerEvent(QTimerEvent *event) override;

    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void itemChange(ItemChange change, const ItemChangeData& value) override;
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;

private:
    void handlePress();
    void handleRelease();
    void ungrabMouse();

private:
    Q_DISABLE_COPY(QQuickMouseArea)
    Q_DECLARE_PRIVATE(QQuickMouseArea)
};

QT_END_NAMESPACE

#endif // QQUICKMOUSEAREA_P_H
