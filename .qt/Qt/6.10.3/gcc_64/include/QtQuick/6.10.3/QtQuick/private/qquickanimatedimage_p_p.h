// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKANIMATEDIMAGE_P_P_H
#define QQUICKANIMATEDIMAGE_P_P_H

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

#include <QtQuick/qtquickglobal.h>

QT_REQUIRE_CONFIG(quick_animatedimage);

#include "qquickimage_p_p.h"

QT_BEGIN_NAMESPACE

class QMovie;
#if QT_CONFIG(qml_network)
class QNetworkReply;
#endif

class QQuickAnimatedImagePrivate : public QQuickImagePrivate
{
    Q_DECLARE_PUBLIC(QQuickAnimatedImage)

public:
    QQuickAnimatedImagePrivate()
      : playing(true), paused(false), oldPlaying(false)
    {
    }

    QQuickPixmap *infoForCurrentFrame(QQmlEngine *engine);
    void setMovie(QMovie *movie);
    void clearCache();

    qreal speed = 1;
    QMovie *movie = nullptr;
    int presetCurrentFrame = 0;
    QMap<int, QQuickPixmap *> frameMap;

#if QT_CONFIG(qml_network)
    QNetworkReply *reply = nullptr;
#endif

    bool playing : 1;
    bool paused : 1;
    bool oldPlaying : 1;
};

QT_END_NAMESPACE

#endif // QQUICKANIMATEDIMAGE_P_P_H
