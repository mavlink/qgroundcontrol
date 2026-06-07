// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKANIMATIONCONTROLLER_H
#define QQUICKANIMATIONCONTROLLER_H

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

#include <qqml.h>
#include <private/qqmlfinalizer_p.h>
#include "qquickanimation_p.h"

QT_BEGIN_NAMESPACE

class QQuickAnimationControllerPrivate;
class Q_QUICK_EXPORT QQuickAnimationController : public QObject, public QQmlFinalizerHook
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(QQuickAnimationController)
    Q_INTERFACES(QQmlFinalizerHook)

    Q_DECLARE_PRIVATE(QQuickAnimationController)
    Q_CLASSINFO("DefaultProperty", "animation")
    QML_NAMED_ELEMENT(AnimationController)
    QML_ADDED_IN_VERSION(2, 0)

    Q_PROPERTY(qreal progress READ progress WRITE setProgress NOTIFY progressChanged)
    Q_PROPERTY(QQuickAbstractAnimation *animation READ animation WRITE setAnimation NOTIFY animationChanged)

public:
    QQuickAnimationController(QObject *parent=nullptr);
    ~QQuickAnimationController();

    qreal progress() const;
    void setProgress(qreal progress);

    QQuickAbstractAnimation *animation() const;
    void setAnimation(QQuickAbstractAnimation *animation);

    void componentFinalized() override;
Q_SIGNALS:
    void progressChanged();
    void animationChanged();
public Q_SLOTS:
    void reload();
    void completeToBeginning();
    void completeToEnd();
private Q_SLOTS:
    void updateProgress();
};

QT_END_NAMESPACE

#endif // QQUICKANIMATIONCONTROLLER_H
