// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QANIMATIONGROUP_P_H
#define QANIMATIONGROUP_P_H

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

#include "qanimationgroup.h"

#include <QtCore/qlist.h>

#include "private/qabstractanimation_p.h"

QT_REQUIRE_CONFIG(animation);

QT_BEGIN_NAMESPACE

class QAnimationGroupPrivate : public QAbstractAnimationPrivate
{
    Q_DECLARE_PUBLIC(QAnimationGroup)
public:
    QAnimationGroupPrivate()
    {
        isGroup = true;
    }

    virtual void animationInsertedAt(qsizetype) { }
    virtual void animationRemoved(qsizetype, QAbstractAnimation *);

    void clear(bool onDestruction);

    void disconnectUncontrolledAnimation(QAbstractAnimation *anim)
    {
        //0 for the signal here because we might be called from the animation destructor
        QObject::disconnect(anim, nullptr, q_func(), SLOT(_q_uncontrolledAnimationFinished()));
    }

    void connectUncontrolledAnimation(QAbstractAnimation *anim)
    {
        QObject::connect(anim, SIGNAL(finished()), q_func(), SLOT(_q_uncontrolledAnimationFinished()));
    }

    QList<QAbstractAnimation *> animations;
};

QT_END_NAMESPACE

#endif //QANIMATIONGROUP_P_H
