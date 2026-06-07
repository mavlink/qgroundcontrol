// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef Q_SPI_APPLICATION_H
#define Q_SPI_APPLICATION_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>
#include <QtCore/QPointer>
#include <QtCore/QQueue>
#include <QtDBus/QDBusConnection>
#include <QtGui/QAccessibleInterface>
Q_MOC_INCLUDE(<QtDBus/QDBusMessage>)

QT_REQUIRE_CONFIG(accessibility);

QT_BEGIN_NAMESPACE

/*
 * Used for the root object.
 *
 * Uses the root object reference and reports its parent as the desktop object.
 */
class QSpiApplicationAdaptor :public QObject
{
    Q_OBJECT

public:
    QSpiApplicationAdaptor(const QDBusConnection &connection, QObject *parent);
    virtual ~QSpiApplicationAdaptor() {}
    void sendEvents(bool active);

Q_SIGNALS:
    void windowActivated(QObject* window, bool active);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private Q_SLOTS:
    void notifyKeyboardListenerCallback(const QDBusMessage& message);
    void notifyKeyboardListenerError(const QDBusError& error, const QDBusMessage& message);

private:
    static QKeyEvent* copyKeyEvent(QKeyEvent*);

    QQueue<std::pair<QPointer<QObject>, QKeyEvent*> > keyEvents;
    QDBusConnection dbusConnection;
};

QT_END_NAMESPACE

#endif
