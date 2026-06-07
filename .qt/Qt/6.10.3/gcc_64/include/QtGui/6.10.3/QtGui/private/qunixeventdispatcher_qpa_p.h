// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QUNIXEVENTDISPATCHER_QPA_H
#define QUNIXEVENTDISPATCHER_QPA_H

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

#include <QtGui/qtguiglobal.h>
#include <QtCore/private/qeventdispatcher_unix_p.h>

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QUnixEventDispatcherQPA : public QEventDispatcherUNIX
{
    Q_OBJECT

public:
    explicit QUnixEventDispatcherQPA(QObject *parent = nullptr);
    ~QUnixEventDispatcherQPA();

    bool processEvents(QEventLoop::ProcessEventsFlags flags) override;
};

QT_END_NAMESPACE

#endif // QUNIXEVENTDISPATCHER_QPA_H
