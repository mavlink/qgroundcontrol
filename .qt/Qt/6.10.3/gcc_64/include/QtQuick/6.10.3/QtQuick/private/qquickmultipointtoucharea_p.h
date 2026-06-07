// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKMULTIPOINTTOUCHAREA_H
#define QQUICKMULTIPOINTTOUCHAREA_H

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

#include <private/qtquickglobal_p.h>

#include <QtQuick/qquickitem.h>

#include <QtGui/qevent.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qstylehints.h>

#include <QtCore/qlist.h>
#include <QtCore/qmap.h>
#include <QtCore/qpointer.h>

#include <QtQml/qqmllist.h>

QT_BEGIN_NAMESPACE

class QQuickMultiPointTouchArea;
class Q_QUICK_EXPORT QQuickTouchPoint : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int pointId READ pointId NOTIFY pointIdChanged)
    Q_PROPERTY(QPointingDeviceUniqueId uniqueId READ uniqueId NOTIFY uniqueIdChanged REVISION(2, 9))
    Q_PROPERTY(bool pressed READ pressed NOTIFY pressedChanged)
    Q_PROPERTY(qreal x READ x NOTIFY xChanged)
    Q_PROPERTY(qreal y READ y NOTIFY yChanged)
    Q_PROPERTY(QSizeF ellipseDiameters READ ellipseDiameters NOTIFY ellipseDiametersChanged REVISION(2, 9))
    Q_PROPERTY(qreal pressure READ pressure NOTIFY pressureChanged)
    Q_PROPERTY(qreal rotation READ rotation NOTIFY rotationChanged REVISION(2, 9))
    Q_PROPERTY(QVector2D velocity READ velocity NOTIFY velocityChanged)
    Q_PROPERTY(QRectF area READ area NOTIFY areaChanged)

    Q_PROPERTY(qreal startX READ startX NOTIFY startXChanged)
    Q_PROPERTY(qreal startY READ startY NOTIFY startYChanged)
    Q_PROPERTY(qreal previousX READ previousX NOTIFY previousXChanged)
    Q_PROPERTY(qreal previousY READ previousY NOTIFY previousYChanged)
    Q_PROPERTY(qreal sceneX READ sceneX NOTIFY sceneXChanged)
    Q_PROPERTY(qreal sceneY READ sceneY NOTIFY sceneYChanged)
    QML_NAMED_ELEMENT(TouchPoint)
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickTouchPoint(bool qmlDefined = true)
        : _qmlDefined(qmlDefined)
    {}

    int pointId() const { return _id; }
    void setPointId(int id);

    QPointingDeviceUniqueId uniqueId() const { return _uniqueId; }
    void setUniqueId(const QPointingDeviceUniqueId &id);

    qreal x() const { return _x; }
    qreal y() const { return _y; }
    void setPosition(QPointF pos);

    QSizeF ellipseDiameters() const { return _ellipseDiameters; }
    void setEllipseDiameters(const QSizeF &d);

    qreal pressure() const { return _pressure; }
    void setPressure(qreal pressure);

    qreal rotation() const { return _rotation; }
    void setRotation(qreal r);

    QVector2D velocity() const { return _velocity; }
    void setVelocity(const QVector2D &velocity);

    QRectF area() const { return _area; }
    void setArea(const QRectF &area);

    bool isQmlDefined() const { return _qmlDefined; }

    bool inUse() const { return _inUse; }
    void setInUse(bool inUse) { _inUse = inUse; }

    bool pressed() const { return _pressed; }
    void setPressed(bool pressed);

    qreal startX() const { return _startX; }
    void setStartX(qreal startX);

    qreal startY() const { return _startY; }
    void setStartY(qreal startY);

    qreal previousX() const { return _previousX; }
    void setPreviousX(qreal previousX);

    qreal previousY() const { return _previousY; }
    void setPreviousY(qreal previousY);

    qreal sceneX() const { return _sceneX; }
    void setSceneX(qreal sceneX);

    qreal sceneY() const { return _sceneY; }
    void setSceneY(qreal sceneY);

Q_SIGNALS:
    void pressedChanged();
    void pointIdChanged();
    Q_REVISION(2, 9) void uniqueIdChanged();
    void xChanged();
    void yChanged();
    Q_REVISION(2, 9) void ellipseDiametersChanged();
    void pressureChanged();
    Q_REVISION(2, 9) void rotationChanged();
    void velocityChanged();
    void areaChanged();
    void startXChanged();
    void startYChanged();
    void previousXChanged();
    void previousYChanged();
    void sceneXChanged();
    void sceneYChanged();

private:
    friend class QQuickMultiPointTouchArea;
    int _id = 0;
    qreal _x = 0.0;
    qreal _y = 0.0;
    qreal _pressure = 0.0;
    qreal _rotation = 0;
    QSizeF _ellipseDiameters;
    QVector2D _velocity;
    QRectF _area;
    bool _qmlDefined;
    bool _inUse = false;    //whether the point is currently in use (only valid when _qmlDefined == true)
    bool _pressed = false;
    qreal _startX = 0.0;
    qreal _startY = 0.0;
    qreal _previousX = 0.0;
    qreal _previousY = 0.0;
    qreal _sceneX = 0.0;
    qreal _sceneY = 0.0;
    QPointingDeviceUniqueId _uniqueId;
};

class QQuickGrabGestureEvent : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<QObject> touchPoints READ touchPoints CONSTANT FINAL)
    Q_PROPERTY(qreal dragThreshold READ dragThreshold CONSTANT FINAL)
    QML_NAMED_ELEMENT(GestureEvent)
    QML_ADDED_IN_VERSION(2, 0)
    QML_UNCREATABLE("GestureEvent is only available in the context of handling the gestureStarted signal from MultiPointTouchArea.")

public:
    QQuickGrabGestureEvent() : _dragThreshold(QGuiApplication::styleHints()->startDragDistance()) {}

    Q_INVOKABLE void grab() { _grab = true; }
    bool wantsGrab() const { return _grab; }

    QQmlListProperty<QObject> touchPoints() {
        return QQmlListProperty<QObject>(this, &_touchPoints);
    }
    qreal dragThreshold() const { return _dragThreshold; }

private:
    friend class QQuickMultiPointTouchArea;
    bool _grab = false;
    qreal _dragThreshold;
    QList<QObject*> _touchPoints;
};

class Q_QUICK_EXPORT QQuickMultiPointTouchArea : public QQuickItem
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(QQuickMultiPointTouchArea)

    Q_PROPERTY(QQmlListProperty<QQuickTouchPoint> touchPoints READ touchPoints CONSTANT)
    Q_PROPERTY(int minimumTouchPoints READ minimumTouchPoints WRITE setMinimumTouchPoints NOTIFY minimumTouchPointsChanged)
    Q_PROPERTY(int maximumTouchPoints READ maximumTouchPoints WRITE setMaximumTouchPoints NOTIFY maximumTouchPointsChanged)
    Q_PROPERTY(bool mouseEnabled READ mouseEnabled WRITE setMouseEnabled NOTIFY mouseEnabledChanged)
    QML_NAMED_ELEMENT(MultiPointTouchArea)
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickMultiPointTouchArea(QQuickItem *parent=nullptr);
    ~QQuickMultiPointTouchArea();

    int minimumTouchPoints() const;
    void setMinimumTouchPoints(int num);
    int maximumTouchPoints() const;
    void setMaximumTouchPoints(int num);
    bool mouseEnabled() const { return _mouseEnabled; }
    void setMouseEnabled(bool arg);

    QQmlListProperty<QQuickTouchPoint> touchPoints() {
        return QQmlListProperty<QQuickTouchPoint>(this, nullptr, QQuickMultiPointTouchArea::touchPoint_append, QQuickMultiPointTouchArea::touchPoint_count, QQuickMultiPointTouchArea::touchPoint_at, nullptr);
    }

    static void touchPoint_append(QQmlListProperty<QQuickTouchPoint> *list, QQuickTouchPoint* touch) {
        QQuickMultiPointTouchArea *q = static_cast<QQuickMultiPointTouchArea*>(list->object);
        q->addTouchPrototype(touch);
    }

    static qsizetype touchPoint_count(QQmlListProperty<QQuickTouchPoint> *list) {
        QQuickMultiPointTouchArea *q = static_cast<QQuickMultiPointTouchArea*>(list->object);
        return q->_touchPrototypes.size();
    }

    static QQuickTouchPoint* touchPoint_at(QQmlListProperty<QQuickTouchPoint> *list, qsizetype index) {
        QQuickMultiPointTouchArea *q = static_cast<QQuickMultiPointTouchArea*>(list->object);
        return q->_touchPrototypes.value(index);
    }

Q_SIGNALS:
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
    void pressed(const QList<QObject*> &touchPoints);
    void updated(const QList<QObject*> &touchPoints);
    void released(const QList<QObject*> &touchPoints);
    void canceled(const QList<QObject*> &touchPoints);
#else
    void pressed(const QList<QObject*> &points);
    void updated(const QList<QObject*> &points);
    void released(const QList<QObject*> &points);
    void canceled(const QList<QObject*> &points);
#endif
    void gestureStarted(QQuickGrabGestureEvent *gesture);
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
    void touchUpdated(const QList<QObject*> &touchPoints);
#else
    void touchUpdated(const QList<QObject*> &points);
#endif
    void minimumTouchPointsChanged();
    void maximumTouchPointsChanged();
    void mouseEnabledChanged();

protected:
    void touchEvent(QTouchEvent *) override;
    bool childMouseEventFilter(QQuickItem *receiver, QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseUngrabEvent() override;
    void touchUngrabEvent() override;

    void addTouchPrototype(QQuickTouchPoint* prototype);
    void addTouchPoint(const QEventPoint *p);
    void addTouchPoint(const QMouseEvent *e);
    void clearTouchLists();

    void updateTouchPoint(QQuickTouchPoint*, const QEventPoint*);
    void updateTouchPoint(QQuickTouchPoint *dtp, const QMouseEvent *e);
    enum class RemapEventPoints { No, ToLocal };
    void updateTouchData(QEvent*, RemapEventPoints remap = RemapEventPoints::No);

    bool sendMouseEvent(QMouseEvent *event);
    bool shouldFilter(QEvent *event);
    void grabGesture(QPointingDevice *dev);
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;
#ifdef Q_OS_MACOS
    void hoverEnterEvent(QHoverEvent *event) override;
    void hoverLeaveEvent(QHoverEvent *event) override;
    void setTouchEventsEnabled(bool enable);
    void itemChange(ItemChange change, const ItemChangeData &data) override;
#endif

private:
    void ungrab(bool normalRelease = false);
    QMap<int,QQuickTouchPoint*> _touchPrototypes;  //TouchPoints defined in QML
    QMap<int,QObject*> _touchPoints;            //All current touch points
    QList<QObject*> _releasedTouchPoints;
    QList<QObject*> _pressedTouchPoints;
    QList<QObject*> _movedTouchPoints;
    int _minimumTouchPoints;
    int _maximumTouchPoints;
    QVector<int> _lastFilterableTouchPointIds;
    QPointer<QQuickTouchPoint> _mouseTouchPoint; // exists when mouse button is down and _mouseEnabled is true; null otherwise
    QEventPoint _mouseQpaTouchPoint; // synthetic QPA touch point to hold state and position of the mouse
    const QPointingDevice *_touchMouseDevice;
    QPointF _mousePos;
    bool _stealMouse;
    bool _mouseEnabled;
};

QT_END_NAMESPACE

#endif // QQUICKMULTIPOINTTOUCHAREA_H
