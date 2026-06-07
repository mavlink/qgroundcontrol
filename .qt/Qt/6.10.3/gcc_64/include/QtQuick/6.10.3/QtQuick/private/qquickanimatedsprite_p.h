// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKANIMATEDSPRITE_P_H
#define QQUICKANIMATEDSPRITE_P_H

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

QT_REQUIRE_CONFIG(quick_sprite);

#include <QtQuick/QQuickItem>
#include <private/qquicksprite_p.h>
#include <QtCore/qelapsedtimer.h>

QT_BEGIN_NAMESPACE

class QSGContext;
class QQuickSprite;
class QQuickSpriteEngine;
class QSGGeometryNode;
class QQuickAnimatedSpritePrivate;
class QSGSpriteNode;
class Q_QUICK_EXPORT QQuickAnimatedSprite : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(bool running READ running WRITE setRunning NOTIFY runningChanged)
    Q_PROPERTY(bool interpolate READ interpolate WRITE setInterpolate NOTIFY interpolateChanged)
    //###try to share similar spriteEngines for less overhead?
    //These properties come out of QQuickSprite, since an AnimatedSprite is a renderer for a single sprite
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(bool reverse READ reverse WRITE setReverse NOTIFY reverseChanged)
    Q_PROPERTY(bool frameSync READ frameSync WRITE setFrameSync NOTIFY frameSyncChanged)
    Q_PROPERTY(int frameCount READ frameCount WRITE setFrameCount NOTIFY frameCountChanged)
    //If frame height or width is not specified, it is assumed to be a single long row of square frames.
    //Otherwise, it can be multiple contiguous rows, when one row runs out the next will be used.
    Q_PROPERTY(int frameHeight READ frameHeight WRITE setFrameHeight NOTIFY frameHeightChanged)
    Q_PROPERTY(int frameWidth READ frameWidth WRITE setFrameWidth NOTIFY frameWidthChanged)
    Q_PROPERTY(int frameX READ frameX WRITE setFrameX NOTIFY frameXChanged)
    Q_PROPERTY(int frameY READ frameY WRITE setFrameY NOTIFY frameYChanged)
    //Precedence order: frameRate, frameDuration
    Q_PROPERTY(qreal frameRate READ frameRate WRITE setFrameRate NOTIFY frameRateChanged RESET resetFrameRate)
    Q_PROPERTY(int frameDuration READ frameDuration WRITE setFrameDuration NOTIFY frameDurationChanged RESET resetFrameDuration)
    //Extra Simple Sprite Stuff
    Q_PROPERTY(int loops READ loops WRITE setLoops NOTIFY loopsChanged)
    Q_PROPERTY(bool paused READ paused WRITE setPaused NOTIFY pausedChanged)
    Q_PROPERTY(int currentFrame READ currentFrame WRITE setCurrentFrame NOTIFY currentFrameChanged)
    Q_PROPERTY(FinishBehavior finishBehavior READ finishBehavior WRITE setFinishBehavior NOTIFY finishBehaviorChanged REVISION(2, 15))
    QML_NAMED_ELEMENT(AnimatedSprite)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickAnimatedSprite(QQuickItem *parent = nullptr);
    enum LoopParameters {
        Infinite = -1
    };
    Q_ENUM(LoopParameters)

    enum FinishBehavior {
        FinishAtInitialFrame,
        FinishAtFinalFrame
    };
    Q_ENUM(FinishBehavior)

    bool running() const;
    bool interpolate() const;
    QUrl source() const;
    bool reverse() const;
    bool frameSync() const;
    int frameCount() const;
    int frameHeight() const;
    int frameWidth() const;
    int frameX() const;
    int frameY() const;
    qreal frameRate() const;
    int frameDuration() const;
    int loops() const;
    bool paused() const;
    int currentFrame() const;
    FinishBehavior finishBehavior() const;
    void setFinishBehavior(FinishBehavior arg);

Q_SIGNALS:

    void pausedChanged(bool arg);
    void runningChanged(bool arg);
    void interpolateChanged(bool arg);

    void sourceChanged(const QUrl &arg);
    void reverseChanged(bool arg);
    void frameSyncChanged(bool arg);
    void frameCountChanged(int arg);
    void frameHeightChanged(int arg);
    void frameWidthChanged(int arg);
    void frameXChanged(int arg);
    void frameYChanged(int arg);
    void frameRateChanged(qreal arg);
    void frameDurationChanged(int arg);
    void loopsChanged(int arg);
    void currentFrameChanged(int arg);
    Q_REVISION(2, 15) void finishBehaviorChanged(QQuickAnimatedSprite::FinishBehavior arg);

    Q_REVISION(2, 12) void finished();

public Q_SLOTS:
    void start();
    void stop();
    void restart() {stop(); start();}
    void advance(int frames=1);
    void pause();
    void resume();

    void setRunning(bool arg);
    void setPaused(bool arg);
    void setInterpolate(bool arg);
    void setSource(const QUrl &arg);
    void setReverse(bool arg);
    void setFrameSync(bool arg);
    void setFrameCount(int arg);
    void setFrameHeight(int arg);
    void setFrameWidth(int arg);
    void setFrameX(int arg);
    void setFrameY(int arg);
    void setFrameRate(qreal arg);
    void setFrameDuration(int arg);
    void resetFrameRate();
    void resetFrameDuration();
    void setLoops(int arg);
    void setCurrentFrame(int arg);

private Q_SLOTS:
    void createEngine();

protected Q_SLOTS:
    void reset();

protected:
    void componentComplete() override;
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;
    void itemChange(ItemChange, const ItemChangeData &) override;

private:
    void maybeUpdate();
    bool isCurrentFrameChangedConnected();
    void prepareNextFrame(QSGSpriteNode *node);
    void reloadImage();
    QSGSpriteNode* initNode();

private:
    Q_DECLARE_PRIVATE(QQuickAnimatedSprite)
};

QT_END_NAMESPACE

#endif // QQUICKANIMATEDSPRITE_P_H
