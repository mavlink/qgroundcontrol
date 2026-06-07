// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QEVENT_P_H
#define QEVENT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists for the convenience
// of other Qt classes. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>
#include <QtCore/qurl.h>
#include <QtGui/qevent.h>
#include <QtGui/qwindow.h>

QT_BEGIN_NAMESPACE

class QPointingDevice;

/*!
    \internal
    \since 6.9

    This class provides a way to store copies of QEvents. QEvents can otherwise
    only be copied by \l{QEvent::clone()}{cloning}, which allocates the copy on
    the heap. By befriending concrete QEvent subclasses, QEventStorage gains
    access to their protected copy constructors.

    It is similar to std::optional, but with a more targeted API.

    Note that by storing an event in QEventStorage, it may be sliced.
*/
template <typename Event>
class QEventStorage
{
    Q_DISABLE_COPY_MOVE(QEventStorage)
    static_assert(std::is_base_of_v<QEvent, Event>);
    union {
        char m_eventNotSet; // could be std::monostate, but don't want to pay for <variant> include
        Event m_event;
    };
    bool m_engaged = false;

    void engage(const Event &e)
    {
        Q_ASSERT(!m_engaged);
        new (&m_event) Event(e);
        m_engaged = true;
    }

    void disengage()
    {
        Q_ASSERT(m_engaged);
        m_event.~Event();
        m_engaged = false;
    }

public:
    QEventStorage() noexcept : m_eventNotSet{} {}
    explicit QEventStorage(const Event &e)
    {
        engage(e);
    }

    ~QEventStorage()
    {
        if (m_engaged)
            disengage();
    }

    explicit operator bool() const noexcept { return m_engaged; }
    bool operator!() const noexcept { return !m_engaged; }

    Event &store(const Event &e)
    {
        if (m_engaged)
            disengage();
        engage(e);
        return m_event;
    }

    Event &storeUnlessAlias(const Event &e)
    {
        if (m_engaged && &e == &m_event)
            return m_event;
        return store(e);
    }

    const Event &operator*() const
    {
        Q_PRE(m_engaged);
        return m_event;
    }

    Event &operator*()
    {
        Q_PRE(m_engaged);
        return m_event;
    }

    const Event *operator->() const
    {
        Q_PRE(m_engaged);
        return &m_event;
    }

    Event *operator->()
    {
        Q_PRE(m_engaged);
        return &m_event;
    }
};

class Q_GUI_EXPORT QMutableTouchEvent : public QTouchEvent
{
public:
    QMutableTouchEvent(QEvent::Type eventType = QEvent::TouchBegin,
                       const QPointingDevice *device = nullptr,
                       Qt::KeyboardModifiers modifiers = Qt::NoModifier,
                       const QList<QEventPoint> &touchPoints = QList<QEventPoint>()) :
        QTouchEvent(eventType, device, modifiers, touchPoints) { }
    ~QMutableTouchEvent() override;

    void setTarget(QObject *target) { m_target = target; }
    void addPoint(const QEventPoint &point);

    static void setTarget(QTouchEvent *e, QObject *target) { e->m_target = target; }
    static void addPoint(QTouchEvent *e, const QEventPoint &point);
};

class Q_GUI_EXPORT QMutableSinglePointEvent : public QSinglePointEvent
{
public:
    QMutableSinglePointEvent(const QSinglePointEvent &other) : QSinglePointEvent(other) {}
    QMutableSinglePointEvent(Type type = QEvent::None, const QPointingDevice *device = nullptr, const QEventPoint &point = QEventPoint(),
                             Qt::MouseButton button = Qt::NoButton, Qt::MouseButtons buttons = Qt::NoButton,
                             Qt::KeyboardModifiers modifiers = Qt::NoModifier,
                             Qt::MouseEventSource source = Qt::MouseEventSynthesizedByQt) :
        QSinglePointEvent(type, device, point, button, buttons, modifiers, source) { }
    ~QMutableSinglePointEvent() override;

    void setSource(Qt::MouseEventSource s) { m_source = s; }

    bool isDoubleClick() { return m_doubleClick; }

    void setDoubleClick(bool d = true) { m_doubleClick = d; }

    static bool isDoubleClick(const QSinglePointEvent *ev)
    {
        return ev->m_doubleClick;
    }

    static void setDoubleClick(QSinglePointEvent *ev, bool d)
    {
        ev->m_doubleClick = d;
    }
};

QT_END_NAMESPACE

#endif // QEVENT_P_H
