// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKQQUICKIMAGEPROVIDER_P_H
#define QQUICKQQUICKIMAGEPROVIDER_P_H

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

#include <private/qtquickglobal_p.h>
#include <private/qobject_p.h>
#include <qquickimageprovider.h>

QT_BEGIN_NAMESPACE

class QQuickImageResponsePrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQuickImageResponse)
public:
    QAtomicInteger<bool> finished = false;

    void _q_finished()
    {
        // synchronizes-with the loadAcquire in QQuickPixmapReader::processJob:
        finished.storeRelease(true);
    }
};


QT_END_NAMESPACE

#endif // QQUICKQQUICKIMAGEPROVIDER_P_H
