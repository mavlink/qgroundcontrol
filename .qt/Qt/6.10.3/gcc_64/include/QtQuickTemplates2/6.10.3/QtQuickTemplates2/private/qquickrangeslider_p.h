// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKRANGESLIDER_P_H
#define QQUICKRANGESLIDER_P_H

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

#include <QtQuickTemplates2/private/qquickcontrol_p.h>

QT_BEGIN_NAMESPACE

class QQuickRangeSliderPrivate;
class QQuickRangeSliderNode;

class Q_QUICKTEMPLATES2_EXPORT QQuickRangeSlider : public QQuickControl
{
    Q_OBJECT
    Q_PROPERTY(qreal from READ from WRITE setFrom NOTIFY fromChanged FINAL)
    Q_PROPERTY(qreal to READ to WRITE setTo NOTIFY toChanged FINAL)
    Q_PROPERTY(QQuickRangeSliderNode *first READ first CONSTANT FINAL)
    Q_PROPERTY(QQuickRangeSliderNode *second READ second CONSTANT FINAL)
    Q_PROPERTY(qreal stepSize READ stepSize WRITE setStepSize NOTIFY stepSizeChanged FINAL)
    Q_PROPERTY(SnapMode snapMode READ snapMode WRITE setSnapMode NOTIFY snapModeChanged FINAL)
    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation NOTIFY orientationChanged FINAL)
    // 2.2 (Qt 5.9)
    Q_PROPERTY(bool live READ live WRITE setLive NOTIFY liveChanged FINAL REVISION(2, 2))
    Q_PROPERTY(bool horizontal READ isHorizontal NOTIFY orientationChanged FINAL REVISION(2, 3))
    // 2.3 (Qt 5.10)
    Q_PROPERTY(bool vertical READ isVertical NOTIFY orientationChanged FINAL REVISION(2, 3))
    // 2.5 (Qt 5.12)
    Q_PROPERTY(qreal touchDragThreshold READ touchDragThreshold WRITE setTouchDragThreshold RESET resetTouchDragThreshold NOTIFY touchDragThresholdChanged FINAL REVISION(2, 5))
    QML_NAMED_ELEMENT(RangeSlider)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickRangeSlider(QQuickItem *parent = nullptr);
    ~QQuickRangeSlider();

    qreal from() const;
    void setFrom(qreal from);

    qreal to() const;
    void setTo(qreal to);

    QQuickRangeSliderNode *first() const;
    QQuickRangeSliderNode *second() const;

    qreal stepSize() const;
    void setStepSize(qreal step);

    enum SnapMode {
        NoSnap,
        SnapAlways,
        SnapOnRelease
    };
    Q_ENUM(SnapMode)

    SnapMode snapMode() const;
    void setSnapMode(SnapMode mode);

    Qt::Orientation orientation() const;
    void setOrientation(Qt::Orientation orientation);

    Q_INVOKABLE void setValues(qreal firstValue, qreal secondValue);

    // 2.2 (Qt 5.9)
    bool live() const;
    void setLive(bool live);

    // 2.3 (Qt 5.10)
    bool isHorizontal() const;
    bool isVertical() const;

    // 2.5 (Qt 5.12)
    qreal touchDragThreshold() const;
    void setTouchDragThreshold(qreal touchDragThreshold);
    void resetTouchDragThreshold();
    Q_REVISION(2, 5) Q_INVOKABLE qreal valueAt(qreal position) const;

Q_SIGNALS:
    void fromChanged();
    void toChanged();
    void stepSizeChanged();
    void snapModeChanged();
    void orientationChanged();
    // 2.2 (Qt 5.9)
    Q_REVISION(2, 2) void liveChanged();
    // 2.5 (Qt 5.12)
    Q_REVISION(2, 5) void touchDragThresholdChanged();

protected:
    void focusInEvent(QFocusEvent *event) override;
    void hoverEnterEvent(QHoverEvent *event) override;
    void hoverMoveEvent(QHoverEvent *event) override;
    void hoverLeaveEvent(QHoverEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
#if QT_CONFIG(quicktemplates2_multitouch)
    void touchEvent(QTouchEvent *event) override;
#endif
    void mirrorChange() override;
    void classBegin() override;
    void componentComplete() override;

#if QT_CONFIG(accessibility)
    QAccessible::Role accessibleRole() const override;
#endif

private:
    friend class QQuickRangeSliderNode;

    Q_DISABLE_COPY(QQuickRangeSlider)
    Q_DECLARE_PRIVATE(QQuickRangeSlider)
};

class QQuickRangeSliderNodePrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickRangeSliderNode : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal value READ value WRITE setValue NOTIFY valueChanged FINAL)
    Q_PROPERTY(qreal position READ position NOTIFY positionChanged FINAL)
    Q_PROPERTY(qreal visualPosition READ visualPosition NOTIFY visualPositionChanged FINAL)
    Q_PROPERTY(QQuickItem *handle READ handle WRITE setHandle NOTIFY handleChanged FINAL)
    Q_PROPERTY(bool pressed READ isPressed WRITE setPressed NOTIFY pressedChanged FINAL)
    // 2.1 (Qt 5.8)
    Q_PROPERTY(bool hovered READ isHovered WRITE setHovered NOTIFY hoveredChanged FINAL REVISION(2, 1))
    // 2.5 (Qt 5.12)
    Q_PROPERTY(qreal implicitHandleWidth READ implicitHandleWidth NOTIFY implicitHandleWidthChanged FINAL REVISION(2, 5))
    Q_PROPERTY(qreal implicitHandleHeight READ implicitHandleHeight NOTIFY implicitHandleHeightChanged FINAL REVISION(2, 5))
    Q_CLASSINFO("DeferredPropertyNames", "handle")
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickRangeSliderNode(qreal value, QQuickRangeSlider *slider);
    ~QQuickRangeSliderNode();

    qreal value() const;
    void setValue(qreal value);

    qreal position() const;
    qreal visualPosition() const;

    QQuickItem *handle() const;
    void setHandle(QQuickItem *handle);

    bool isPressed() const;
    void setPressed(bool pressed);

    // 2.1 (Qt 5.8)
    bool isHovered() const;
    void setHovered(bool hovered);

    // 2.5 (Qt 5.12)
    qreal implicitHandleWidth() const;
    qreal implicitHandleHeight() const;

public Q_SLOTS:
    void increase();
    void decrease();

Q_SIGNALS:
    void valueChanged();
    void positionChanged();
    void visualPositionChanged();
    void handleChanged();
    void pressedChanged();
    // 2.1 (Qt 5.8)
    Q_REVISION(2, 1) void hoveredChanged();
    // 2.5 (Qt 5.12)
    /*Q_REVISION(2, 5)*/ void moved();
    /*Q_REVISION(2, 5)*/ void implicitHandleWidthChanged();
    /*Q_REVISION(2, 5)*/ void implicitHandleHeightChanged();

private:
    Q_DISABLE_COPY(QQuickRangeSliderNode)
    Q_DECLARE_PRIVATE(QQuickRangeSliderNode)
};

QT_END_NAMESPACE

#endif // QQUICKRANGESLIDER_P_H
