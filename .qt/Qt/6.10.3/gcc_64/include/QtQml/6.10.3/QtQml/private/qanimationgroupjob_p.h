// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QANIMATIONGROUPJOB_P_H
#define QANIMATIONGROUPJOB_P_H

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

#include <QtQml/private/qabstractanimationjob_p.h>
#include <QtQml/private/qdoubleendedlist_p.h>
#include <QtCore/qdebug.h>

QT_REQUIRE_CONFIG(qml_animation);

QT_BEGIN_NAMESPACE

class Q_QML_EXPORT QAnimationGroupJob : public QAbstractAnimationJob
{
    Q_DISABLE_COPY(QAnimationGroupJob)
public:
    using Children = QDoubleEndedList<QAbstractAnimationJob>;

    QAnimationGroupJob();
    ~QAnimationGroupJob() override;

    void appendAnimation(QAbstractAnimationJob *animation);
    void prependAnimation(QAbstractAnimationJob *animation);
    void removeAnimation(QAbstractAnimationJob *animation);

    Children *children() { return &m_children; }
    const Children *children() const { return &m_children; }

    virtual void clear();

    //called by QAbstractAnimationJob
    virtual void uncontrolledAnimationFinished(QAbstractAnimationJob *animation);
protected:
    void topLevelAnimationLoopChanged() override;

    virtual void animationInserted(QAbstractAnimationJob*) { }
    virtual void animationRemoved(QAbstractAnimationJob*, QAbstractAnimationJob*, QAbstractAnimationJob*);

    //TODO: confirm location of these (should any be moved into QAbstractAnimationJob?)
    void resetUncontrolledAnimationsFinishTime();
    void resetUncontrolledAnimationFinishTime(QAbstractAnimationJob *anim);
    int uncontrolledAnimationFinishTime(const QAbstractAnimationJob *anim) const
    {
        return anim->m_uncontrolledFinishTime;
    }
    void setUncontrolledAnimationFinishTime(QAbstractAnimationJob *anim, int time);

    void debugChildren(QDebug d) const;

    void ungroupChild(QAbstractAnimationJob *animation);
    void handleAnimationRemoved(QAbstractAnimationJob *animation);

    Children m_children;
};

QT_END_NAMESPACE

#endif //QANIMATIONGROUPJOB_P_H
