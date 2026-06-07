// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKDIAL_P_H
#define QQUICKDIAL_P_H

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

#include <QtCore/qvariant.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQuickTemplates2/private/qquickcontrol_p.h>

QT_BEGIN_NAMESPACE

class QQuickDialAttached;
class QQuickDialPrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickDial : public QQuickControl
{
    Q_OBJECT
    Q_PROPERTY(qreal from READ from WRITE setFrom NOTIFY fromChanged FINAL)
    Q_PROPERTY(qreal to READ to WRITE setTo NOTIFY toChanged FINAL)
    Q_PROPERTY(qreal value READ value WRITE setValue NOTIFY valueChanged FINAL)
    Q_PROPERTY(qreal position READ position NOTIFY positionChanged FINAL)
    Q_PROPERTY(qreal angle READ angle NOTIFY angleChanged FINAL)
    Q_PROPERTY(qreal startAngle READ startAngle WRITE setStartAngle NOTIFY startAngleChanged FINAL REVISION(6, 6))
    Q_PROPERTY(qreal endAngle READ endAngle WRITE setEndAngle NOTIFY endAngleChanged FINAL REVISION(6, 6))
    Q_PROPERTY(qreal stepSize READ stepSize WRITE setStepSize NOTIFY stepSizeChanged FINAL)
    Q_PROPERTY(SnapMode snapMode READ snapMode WRITE setSnapMode NOTIFY snapModeChanged FINAL)
    Q_PROPERTY(bool wrap READ wrap WRITE setWrap NOTIFY wrapChanged FINAL)
    Q_PROPERTY(bool pressed READ isPressed NOTIFY pressedChanged FINAL)
    Q_PROPERTY(QQuickItem *handle READ handle WRITE setHandle NOTIFY handleChanged FINAL)
    // 2.2 (Qt 5.9)
    Q_PROPERTY(bool live READ live WRITE setLive NOTIFY liveChanged FINAL REVISION(2, 2))
    // 2.5 (Qt 5.12)
    Q_PROPERTY(InputMode inputMode READ inputMode WRITE setInputMode NOTIFY inputModeChanged FINAL REVISION(2, 5))
    Q_CLASSINFO("DeferredPropertyNames", "background,handle")
    QML_NAMED_ELEMENT(Dial)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickDial(QQuickItem *parent = nullptr);

    qreal from() const;
    void setFrom(qreal from);

    qreal to() const;
    void setTo(qreal to);

    qreal value() const;
    void setValue(qreal value);

    qreal position() const;

    qreal angle() const;

    qreal stepSize() const;
    void setStepSize(qreal step);

    qreal startAngle() const;
    void setStartAngle(qreal startAngle);

    qreal endAngle() const;
    void setEndAngle(qreal endAngle);

    enum SnapMode {
        NoSnap,
        SnapAlways,
        SnapOnRelease
    };
    Q_ENUM(SnapMode)

    SnapMode snapMode() const;
    void setSnapMode(SnapMode mode);

    enum InputMode {
        Circular,
        Horizontal,
        Vertical,
    };
    Q_ENUM(InputMode)

    enum WrapDirection {
        Clockwise,
        CounterClockwise
    };
    Q_ENUM(WrapDirection)

    bool wrap() const;
    void setWrap(bool wrap);

    bool isPressed() const;
    void setPressed(bool pressed);

    QQuickItem *handle() const;
    void setHandle(QQuickItem *handle);

    // 2.2 (Qt 5.9)
    bool live() const;
    void setLive(bool live);

    // 2.5 (Qt 5.12)
    InputMode inputMode() const;
    void setInputMode(InputMode mode);

public Q_SLOTS:
    void increase();
    void decrease();

Q_SIGNALS:
    void fromChanged();
    void toChanged();
    void valueChanged();
    void positionChanged();
    void angleChanged();
    void stepSizeChanged();
    void snapModeChanged();
    void wrapChanged();
    void pressedChanged();
    void handleChanged();
    // 2.2 (Qt 5.9)
    Q_REVISION(2, 2) void moved();
    Q_REVISION(2, 2) void liveChanged();
    // 2.5 (Qt 5.12)
    Q_REVISION(2, 5) void inputModeChanged();
    Q_REVISION(6, 6) void startAngleChanged();
    Q_REVISION(6, 6) void endAngleChanged();
    Q_REVISION(6, 6) void wrapped(WrapDirection);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
#if QT_CONFIG(quicktemplates2_multitouch)
    void touchEvent(QTouchEvent *event) override;
#endif
#if QT_CONFIG(wheelevent)
    void wheelEvent(QWheelEvent *event) override;
#endif

    void mirrorChange() override;
    void componentComplete() override;

#if QT_CONFIG(accessibility)
    void accessibilityActiveChanged(bool active) override;
    QAccessible::Role accessibleRole() const override;
#endif

private:
    Q_DISABLE_COPY(QQuickDial)
    Q_DECLARE_PRIVATE(QQuickDial)
};

QT_END_NAMESPACE

#endif // QQUICKDIAL_P_H
