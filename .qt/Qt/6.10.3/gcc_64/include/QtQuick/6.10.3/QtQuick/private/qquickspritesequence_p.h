// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKSPRITESEQUENCE_P_H
#define QQUICKSPRITESEQUENCE_P_H

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

QT_BEGIN_NAMESPACE

class QSGContext;
class QQuickSprite;
class QQuickSpriteEngine;
class QQuickSpriteSequencePrivate;
class QSGSpriteNode;
class Q_QUICK_EXPORT QQuickSpriteSequence : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(bool running READ running WRITE setRunning NOTIFY runningChanged)
    Q_PROPERTY(bool interpolate READ interpolate WRITE setInterpolate NOTIFY interpolateChanged)
    Q_PROPERTY(QString goalSprite READ goalSprite WRITE setGoalSprite NOTIFY goalSpriteChanged)
    Q_PROPERTY(QString currentSprite READ currentSprite NOTIFY currentSpriteChanged)
    //###try to share similar spriteEngines for less overhead?
    Q_PROPERTY(QQmlListProperty<QQuickSprite> sprites READ sprites)
    Q_CLASSINFO("DefaultProperty", "sprites")
    QML_NAMED_ELEMENT(SpriteSequence)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickSpriteSequence(QQuickItem *parent = nullptr);

    QQmlListProperty<QQuickSprite> sprites();

    bool running() const;
    bool interpolate() const;
    QString goalSprite() const;
    QString currentSprite() const;

Q_SIGNALS:

    void runningChanged(bool arg);
    void interpolateChanged(bool arg);
    void goalSpriteChanged(const QString &arg);
    void currentSpriteChanged(const QString &arg);

public Q_SLOTS:

    void jumpTo(const QString &sprite);
    void setGoalSprite(const QString &sprite);
    void setRunning(bool arg);
    void setInterpolate(bool arg);

private Q_SLOTS:
    void createEngine();

protected:
    void reset();
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;
private:
    void prepareNextFrame(QSGSpriteNode *node);
    QSGSpriteNode* initNode();

private:
    Q_DECLARE_PRIVATE(QQuickSpriteSequence)
};

QT_END_NAMESPACE

#endif // QQUICKSPRITESEQUENCE_P_H
