// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKABSTRACTBUTTON_P_P_H
#define QQUICKABSTRACTBUTTON_P_P_H

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

#include <QtQuickTemplates2/private/qquickabstractbutton_p.h>
#include <QtQuickTemplates2/private/qquickcontrol_p_p.h>
#if QT_CONFIG(shortcut)
#  include <QtGui/qkeysequence.h>
#endif

#include <QtCore/qpointer.h>

#include <QtQml/private/qqmlpropertyutils_p.h>

QT_BEGIN_NAMESPACE

class QQuickAction;
class QQuickButtonGroup;

class Q_QUICKTEMPLATES2_EXPORT QQuickAbstractButtonPrivate : public QQuickControlPrivate
{
public:
    Q_DECLARE_PUBLIC(QQuickAbstractButton)

    static QQuickAbstractButtonPrivate *get(QQuickAbstractButton *button)
    {
        return button->d_func();
    }

    QPointF centerPressPoint() const;
    void setPressPoint(const QPointF &point);
    void setMovePoint(const QPointF &point);

    bool handlePress(const QPointF &point, ulong timestamp) override;
    bool handleMove(const QPointF &point, ulong timestamp) override;
    bool handleRelease(const QPointF &point, ulong timestamp) override;
    void handleUngrab() override;

    virtual bool acceptKeyClick(Qt::Key key) const;
    virtual void accessiblePressAction();

    bool isPressAndHoldConnected();
    bool isDoubleClickConnected();
    void startPressAndHold();
    void stopPressAndHold();

    void startRepeatDelay();
    void startPressRepeat();
    void stopPressRepeat();

#if QT_CONFIG(shortcut)
    void grabShortcut();
    void ungrabShortcut();
#endif

    QQuickAbstractButton *findCheckedButton() const;
    QList<QQuickAbstractButton *> findExclusiveButtons() const;

    void actionTextChange();
    void setText(const QString &text, QQml::PropertyUtils::State propertyState);
    void init();

    void updateEffectiveIcon();

    void click();
    void trigger(bool doubleClick = false);
    void toggle(bool value);

    void cancelIndicator();
    void executeIndicator(bool complete = false);

    void itemImplicitWidthChanged(QQuickItem *item) override;
    void itemImplicitHeightChanged(QQuickItem *item) override;
    void itemDestroyed(QQuickItem *item) override;

    // copied from qabstractbutton.cpp
    static const int AUTO_REPEAT_DELAY = 300;
    static const int AUTO_REPEAT_INTERVAL = 100;

    bool explicitText = false;
    bool down = false;
    bool explicitDown = false;
    bool pressed = false;
    bool keepPressed = false;
    bool checked = false;
    bool checkable = false;
    bool autoExclusive = false;
    bool autoRepeat = false;
    bool wasHeld = false;
    bool wasDoubleClick = false;
    int holdTimer = 0;
    int delayTimer = 0;
    int repeatTimer = 0;
    int repeatDelay = AUTO_REPEAT_DELAY;
    int repeatInterval = AUTO_REPEAT_INTERVAL;
    int animateTimer = 0;
#if QT_CONFIG(shortcut)
    int shortcutId = 0;
    QKeySequence shortcut;
#endif
    qreal lastTouchReleaseTimestamp = 0;
    QString text;
    QQuickIcon icon;
    QQuickIcon effectiveIcon;
    QPointF pressPoint;
    QPointF movePoint;
    Qt::MouseButtons pressButtons = Qt::NoButton;
    QQuickAbstractButton::Display display = QQuickAbstractButton::TextBesideIcon;
    QQuickDeferredPointer<QQuickItem> indicator;
    QQuickButtonGroup *group = nullptr;
    QPointer<QQuickAction> action;
};

QT_END_NAMESPACE

#endif // QQUICKABSTRACTBUTTON_P_P_H
