// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTESTWHEEL_H
#define QTESTWHEEL_H

#if 0
// inform syncqt
#pragma qt_no_master_include
#endif

#include <QtTest/qttestglobal.h>
#include <QtTest/qtestassert.h>
#include <QtTest/qtestsystem.h>
#include <QtTest/qtestspontaneevent.h>
#include <QtCore/qpoint.h>
#include <QtCore/qstring.h>
#include <QtCore/qpointer.h>
#include <QtGui/qevent.h>
#include <QtGui/qwindow.h>

QT_BEGIN_NAMESPACE

Q_GUI_EXPORT void qt_handleWheelEvent(QWindow *window, const QPointF &local,
                                      const QPointF &global, QPoint pixelDelta,
                                      QPoint angleDelta, Qt::KeyboardModifiers mods, Qt::ScrollPhase phase);

namespace QTest
{
    /*! \internal
        This function creates a mouse wheel event and calls
        QWindowSystemInterface::handleWheelEvent().
        \a window is the window that should be receiving the event and \a pos
        provides the location of the event in the window's local coordinates.
        \a angleDelta contains the wheel rotation angle, while \a pixelDelta
        contains the scrolling distance in pixels on screen.
        The keyboard states at the time of the event are specified by \a stateKey.
        The scrolling phase of the event is specified by \a phase.
    */
    [[maybe_unused]] static void wheelEvent(QWindow *window, QPointF pos,
                                            QPoint angleDelta, QPoint pixelDelta = QPoint(0, 0),
                                            Qt::KeyboardModifiers stateKey = Qt::NoModifier,
                                            Qt::ScrollPhase phase = Qt::NoScrollPhase)
    {
        QTEST_ASSERT(window);

        // pos is in window local coordinates
        const QSize windowSize = window->geometry().size();
        if (windowSize.width() <= pos.x() || windowSize.height() <= pos.y()) {
            qWarning("Mouse event at %d, %d occurs outside target window (%dx%d).",
                        static_cast<int>(pos.x()), static_cast<int>(pos.y()), windowSize.width(), windowSize.height());
        }

        if (pos.isNull())
            pos = QPoint(window->width() / 2, window->height() / 2);

        QPointF global = window->mapToGlobal(pos);
        QPointer<QWindow> w(window);

        if (angleDelta.isNull() && pixelDelta.isNull())
            qWarning("No angle or pixel delta specified.");

        qt_handleWheelEvent(w, pos, global, pixelDelta, angleDelta, stateKey, phase);
        qApp->processEvents();
    }
}

QT_END_NAMESPACE

#endif // QTESTWHEEL_H
