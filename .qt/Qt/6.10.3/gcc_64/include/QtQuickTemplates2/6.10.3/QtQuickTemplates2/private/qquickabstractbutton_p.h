// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKABSTRACTBUTTON_P_H
#define QQUICKABSTRACTBUTTON_P_H

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
#include <QtQuickTemplates2/private/qquickicon_p.h>

QT_BEGIN_NAMESPACE

class QQuickAction;
class QQuickAbstractButtonPrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickAbstractButton : public QQuickControl
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText RESET resetText NOTIFY textChanged FINAL)
    Q_PROPERTY(bool down READ isDown WRITE setDown NOTIFY downChanged RESET resetDown FINAL)
    Q_PROPERTY(bool pressed READ isPressed NOTIFY pressedChanged FINAL)
    Q_PROPERTY(bool checked READ isChecked WRITE setChecked NOTIFY checkedChanged FINAL)
    Q_PROPERTY(bool checkable READ isCheckable WRITE setCheckable NOTIFY checkableChanged FINAL)
    Q_PROPERTY(bool autoExclusive READ autoExclusive WRITE setAutoExclusive NOTIFY autoExclusiveChanged FINAL)
    Q_PROPERTY(bool autoRepeat READ autoRepeat WRITE setAutoRepeat NOTIFY autoRepeatChanged FINAL)
    Q_PROPERTY(QQuickItem *indicator READ indicator WRITE setIndicator NOTIFY indicatorChanged FINAL)
    // 2.3 (Qt 5.10)
    Q_PROPERTY(QQuickIcon icon READ icon WRITE setIcon NOTIFY iconChanged FINAL REVISION(2, 3))
    Q_PROPERTY(Display display READ display WRITE setDisplay NOTIFY displayChanged FINAL REVISION(2, 3))
    Q_PROPERTY(QQuickAction *action READ action WRITE setAction NOTIFY actionChanged FINAL REVISION(2, 3))
    // 2.4 (Qt 5.11)
    Q_PROPERTY(int autoRepeatDelay READ autoRepeatDelay WRITE setAutoRepeatDelay NOTIFY autoRepeatDelayChanged FINAL REVISION(2, 4))
    Q_PROPERTY(int autoRepeatInterval READ autoRepeatInterval WRITE setAutoRepeatInterval NOTIFY autoRepeatIntervalChanged FINAL REVISION(2, 4))
    Q_PROPERTY(qreal pressX READ pressX NOTIFY pressXChanged FINAL REVISION(2, 4))
    Q_PROPERTY(qreal pressY READ pressY NOTIFY pressYChanged FINAL REVISION(2, 4))
    // 2.5 (Qt 5.12)
    Q_PROPERTY(qreal implicitIndicatorWidth READ implicitIndicatorWidth NOTIFY implicitIndicatorWidthChanged FINAL REVISION(2, 5))
    Q_PROPERTY(qreal implicitIndicatorHeight READ implicitIndicatorHeight NOTIFY implicitIndicatorHeightChanged FINAL REVISION(2, 5))
    Q_CLASSINFO("DeferredPropertyNames", "background,contentItem,indicator")
    QML_NAMED_ELEMENT(AbstractButton)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickAbstractButton(QQuickItem *parent = nullptr);
    ~QQuickAbstractButton();

    QString text() const;
    void setText(const QString &text);
    void resetText();

    bool isDown() const;
    void setDown(bool down);
    void resetDown();

    bool isPressed() const;
    void setPressed(bool pressed);

    bool isChecked() const;
    void setChecked(bool checked);

    bool isCheckable() const;
    void setCheckable(bool checkable);

    bool autoExclusive() const;
    void setAutoExclusive(bool exclusive);

    bool autoRepeat() const;
    void setAutoRepeat(bool repeat);

    QQuickItem *indicator() const;
    void setIndicator(QQuickItem *indicator);

    // 2.3 (Qt 5.10)
    QQuickIcon icon() const;
    void setIcon(const QQuickIcon &icon);

    enum Display {
        IconOnly,
        TextOnly,
        TextBesideIcon,
        TextUnderIcon
    };
    Q_ENUM(Display)

    Display display() const;
    void setDisplay(Display display);

    QQuickAction *action() const;
    void setAction(QQuickAction *action);

#if QT_CONFIG(shortcut)
    QKeySequence shortcut() const;
    void setShortcut(const QKeySequence &shortcut);
#endif

    // 2.4 (Qt 5.11)
    int autoRepeatDelay() const;
    void setAutoRepeatDelay(int delay);

    int autoRepeatInterval() const;
    void setAutoRepeatInterval(int interval);

    qreal pressX() const;
    qreal pressY() const;

    // 2.5 (Qt 5.12)
    qreal implicitIndicatorWidth() const;
    qreal implicitIndicatorHeight() const;

public Q_SLOTS:
    void toggle();
    Q_REVISION(6, 8) void click();
    Q_REVISION(6, 8) void animateClick();

Q_SIGNALS:
    void pressed();
    void released();
    void canceled();
    void clicked();
    void pressAndHold();
    void doubleClicked();
    void textChanged();
    void downChanged();
    void pressedChanged();
    void checkedChanged();
    void checkableChanged();
    void autoExclusiveChanged();
    void autoRepeatChanged();
    void indicatorChanged();
    // 2.2 (Qt 5.9)
    Q_REVISION(2, 2) void toggled();
    // 2.3 (Qt 5.10)
    Q_REVISION(2, 3) void iconChanged();
    Q_REVISION(2, 3) void displayChanged();
    Q_REVISION(2, 3) void actionChanged();
    // 2.4 (Qt 5.11)
    Q_REVISION(2, 4) void autoRepeatDelayChanged();
    Q_REVISION(2, 4) void autoRepeatIntervalChanged();
    Q_REVISION(2, 4) void pressXChanged();
    Q_REVISION(2, 4) void pressYChanged();
    // 2.5 (Qt 5.12)
    Q_REVISION(2, 5) void implicitIndicatorWidthChanged();
    Q_REVISION(2, 5) void implicitIndicatorHeightChanged();

protected:
    QQuickAbstractButton(QQuickAbstractButtonPrivate &dd, QQuickItem *parent);

    void componentComplete() override;

    bool event(QEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void timerEvent(QTimerEvent *event) override;

    void itemChange(ItemChange change, const ItemChangeData &value) override;

    enum ButtonChange {
        ButtonCheckedChange,
        ButtonCheckableChange,
        ButtonPressedChanged,
        ButtonTextChange
    };
    virtual void buttonChange(ButtonChange change);

    virtual void nextCheckState();

#if QT_CONFIG(accessibility)
    void accessibilityActiveChanged(bool active) override;
    QAccessible::Role accessibleRole() const override;
    Q_INVOKABLE void accessiblePressAction();
#endif

private:
    Q_DISABLE_COPY(QQuickAbstractButton)
    Q_DECLARE_PRIVATE(QQuickAbstractButton)
};

QT_END_NAMESPACE

#endif // QQUICKABSTRACTBUTTON_P_H
