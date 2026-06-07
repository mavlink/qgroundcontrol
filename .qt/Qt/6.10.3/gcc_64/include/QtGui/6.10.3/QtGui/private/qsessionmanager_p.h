// Copyright (C) 2013 Samuel Gaist <samuel.gaist@edeltech.ch>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSESSIONMANAGER_P_H
#define QSESSIONMANAGER_P_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include <private/qobject_p.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>

#ifndef QT_NO_SESSIONMANAGER

QT_BEGIN_NAMESPACE

class QPlatformSessionManager;

class Q_GUI_EXPORT QSessionManagerPrivate : public QObjectPrivate
{
public:
    QSessionManagerPrivate(const QString &id,
                           const QString &key);

    ~QSessionManagerPrivate();

    QPlatformSessionManager *platformSessionManager;
};

QT_END_NAMESPACE

#endif // QT_NO_SESSIONMANAGER

#endif // QSESSIONMANAGER_P_H
