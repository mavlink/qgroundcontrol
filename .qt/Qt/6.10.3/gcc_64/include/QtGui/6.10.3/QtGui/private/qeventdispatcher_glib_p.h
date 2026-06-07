// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QEVENTDISPATCHER_GLIB_QPA_P_H
#define QEVENTDISPATCHER_GLIB_QPA_P_H

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

#include <QtCore/private/qeventdispatcher_glib_p.h>
#include <QtGui/qtguiglobal.h>

typedef struct _GMainContext GMainContext;

QT_BEGIN_NAMESPACE
class QPAEventDispatcherGlibPrivate;

class Q_GUI_EXPORT QPAEventDispatcherGlib : public QEventDispatcherGlib
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QPAEventDispatcherGlib)

public:
    explicit QPAEventDispatcherGlib(QObject *parent = nullptr);
    ~QPAEventDispatcherGlib();

    bool processEvents(QEventLoop::ProcessEventsFlags flags) override;
    QEventLoop::ProcessEventsFlags m_flags;
};

struct GUserEventSource;

class QPAEventDispatcherGlibPrivate : public QEventDispatcherGlibPrivate
{
    Q_DECLARE_PUBLIC(QPAEventDispatcherGlib)
public:
    QPAEventDispatcherGlibPrivate(GMainContext *context = nullptr);
    ~QPAEventDispatcherGlibPrivate() override;

    GUserEventSource *userEventSource;
};


QT_END_NAMESPACE

#endif // QEVENTDISPATCHER_GLIB_QPA_P_H
