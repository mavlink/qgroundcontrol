// Copyright (C) 2014 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Volker Krause <volker.krause@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#ifndef QHOOKS_H
#define QHOOKS_H

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

#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QObject;

namespace QHooks {

enum HookIndex {
    HookDataVersion = 0,
    HookDataSize = 1,
    QtVersion = 2,
    AddQObject = 3,
    RemoveQObject = 4,
    Startup = 5,
    TypeInformationVersion = 6,
    LastHookIndex
};

typedef void(*AddQObjectCallback)(QObject*);
typedef void(*RemoveQObjectCallback)(QObject*);
typedef void(*StartupCallback)();

}

extern quintptr Q_CORE_EXPORT qtHookData[];

QT_END_NAMESPACE

#endif
