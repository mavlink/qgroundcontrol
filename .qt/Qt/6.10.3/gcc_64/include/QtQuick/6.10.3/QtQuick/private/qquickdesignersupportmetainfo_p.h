// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef DESIGNERSUPPORTMETAINFO_H
#define DESIGNERSUPPORTMETAINFO_H

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

#include "qquickdesignersupport_p.h"

#include <QObject>
#include <QByteArray>

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QQuickDesignerSupportMetaInfo
{
public:
    static bool isSubclassOf(QObject *object, const QByteArray &superTypeName);
    static void registerNotifyPropertyChangeCallBack(void (*callback)(QObject *, const QQuickDesignerSupport::PropertyName &));
    static void registerMockupObject(const char *uri, int versionMajor, int versionMinor, const char *qmlName);
};

QT_END_NAMESPACE

#endif // DESIGNERSUPPORTMETAINFO_H
