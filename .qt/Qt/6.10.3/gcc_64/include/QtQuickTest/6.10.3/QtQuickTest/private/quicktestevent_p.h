// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QUICKTESTEVENT_P_H
#define QUICKTESTEVENT_P_H

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

#include <QtQuickTest/private/quicktestglobal_p.h>

#include <QtCore/qobject.h>
#include <QtGui/QWindow>
#include <QtQml/qqml.h>
#include <QtTest/qtesttouch.h>

QT_BEGIN_NAMESPACE

class QuickTestEvent;
class Q_QMLTEST_EXPORT QQuickTouchEventSequence : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(1, 0)

public:
    explicit QQuickTouchEventSequence(QuickTestEvent *testEvent, QObject *item = nullptr);
public Q_SLOTS:
    QObject* press(int touchId, QObject *item, qreal x, qreal y);
    QObject* move(int touchId, QObject *item, qreal x, qreal y);
    QObject* release(int touchId, QObject *item, qreal x, qreal y);
    QObject* stationary(int touchId);
    QObject* commit();

private:
    QTest::QTouchEventSequence m_sequence;
    QuickTestEvent * const m_testEvent;
};

class Q_QMLTEST_EXPORT QuickTestEvent : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int defaultMouseDelay READ defaultMouseDelay FINAL)
    QML_NAMED_ELEMENT(TestEvent)
    QML_ADDED_IN_VERSION(1, 0)
public:
    QuickTestEvent(QObject *parent = nullptr);
    ~QuickTestEvent() override;
    int defaultMouseDelay() const;

public Q_SLOTS:
    bool keyPress(int key, int modifiers, int delay);
    bool keyRelease(int key, int modifiers, int delay);
    bool keyClick(int key, int modifiers, int delay);

    bool keyPressChar(const QString &character, int modifiers, int delay);
    bool keyReleaseChar(const QString &character, int modifiers, int delay);
    bool keyClickChar(const QString &character, int modifiers, int delay);

    Q_REVISION(1, 2) bool keySequence(const QVariant &keySequence);

    bool mousePress(QObject *item, qreal x, qreal y, int button,
                    int modifiers, int delay);
    bool mouseRelease(QObject *item, qreal x, qreal y, int button,
                      int modifiers, int delay);
    bool mouseClick(QObject *item, qreal x, qreal y, int button,
                    int modifiers, int delay);
    bool mouseDoubleClick(QObject *item, qreal x, qreal y, int button,
                          int modifiers, int delay);
    bool mouseDoubleClickSequence(QObject *item, qreal x, qreal y, int button,
                          int modifiers, int delay);
    bool mouseMove(QObject *item, qreal x, qreal y, int delay, int buttons, int modifiers);

#if QT_CONFIG(wheelevent)
    bool mouseWheel(QObject *item, qreal x, qreal y, int buttons,
               int modifiers, int xDelta, int yDelta, int delay);
#endif

    QQuickTouchEventSequence *touchEvent(QObject *item = nullptr);
private:
    QWindow *eventWindow(QObject *item = nullptr);
    QWindow *activeWindow();
    QPointingDevice *touchDevice();

    Qt::MouseButtons m_pressedButtons;

    friend class QQuickTouchEventSequence;
};

QT_END_NAMESPACE

#endif
