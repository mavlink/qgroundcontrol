// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKANIMATEDIMAGE_P_H
#define QQUICKANIMATEDIMAGE_P_H

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

QT_REQUIRE_CONFIG(quick_animatedimage);

#include "qquickimage_p.h"

QT_BEGIN_NAMESPACE

class QMovie;
class QQuickAnimatedImagePrivate;

class Q_QUICK_EXPORT QQuickAnimatedImage : public QQuickImage
{
    Q_OBJECT

    Q_PROPERTY(bool playing READ isPlaying WRITE setPlaying NOTIFY playingChanged)
    Q_PROPERTY(bool paused READ isPaused WRITE setPaused NOTIFY pausedChanged)
    Q_PROPERTY(int currentFrame READ currentFrame WRITE setCurrentFrame NOTIFY frameChanged)
    Q_PROPERTY(int frameCount READ frameCount NOTIFY frameCountChanged)
    Q_PROPERTY(qreal speed READ speed WRITE setSpeed NOTIFY speedChanged REVISION(2, 11))

    QML_NAMED_ELEMENT(AnimatedImage)
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickAnimatedImage(QQuickItem *parent=nullptr);
    ~QQuickAnimatedImage();

    bool isPlaying() const;
    void setPlaying(bool play);

    bool isPaused() const;
    void setPaused(bool pause);

    int currentFrame() const override;
    void setCurrentFrame(int frame) override;

    int frameCount() const override;

    qreal speed() const;
    void setSpeed(qreal speed);

    // Extends QQuickImage's src property
    void setSource(const QUrl&) override;

Q_SIGNALS:
    void playingChanged();
    void pausedChanged();
    void frameChanged();
    void currentFrameChanged();
    void frameCountChanged();
    Q_REVISION(2, 11) void speedChanged();

private Q_SLOTS:
    void movieUpdate();
    void movieRequestFinished();
    void playingStatusChanged();
    void onCacheChanged();

protected:
    void load() override;
    void componentComplete() override;

private:
    Q_DISABLE_COPY(QQuickAnimatedImage)
    Q_DECLARE_PRIVATE(QQuickAnimatedImage)
};

QT_END_NAMESPACE

#endif // QQUICKANIMATEDIMAGE_P_H
