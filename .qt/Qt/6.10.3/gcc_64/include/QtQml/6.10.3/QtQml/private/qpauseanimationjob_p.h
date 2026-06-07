// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QPAUSEANIMATIONJOB_P_H
#define QPAUSEANIMATIONJOB_P_H

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

#include <private/qanimationgroupjob_p.h>

QT_REQUIRE_CONFIG(qml_animation);

QT_BEGIN_NAMESPACE

class Q_QML_EXPORT QPauseAnimationJob : public QAbstractAnimationJob
{
    Q_DISABLE_COPY(QPauseAnimationJob)
public:
    explicit QPauseAnimationJob(int duration = 250);
    ~QPauseAnimationJob() override;

    int duration() const override;
    void setDuration(int msecs);

protected:
    void updateCurrentTime(int) override;
    void debugAnimation(QDebug d) const override;

private:
    //definition
    int m_duration;
};

QT_END_NAMESPACE

#endif // QPAUSEANIMATIONJOB_P_H
