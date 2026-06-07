// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QFRAME_P_H
#define QFRAME_P_H

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

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "private/qwidget_p.h"
#include "qframe.h"

QT_BEGIN_NAMESPACE

class QFramePrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QFrame)
public:
    QFramePrivate();
    ~QFramePrivate();

    void        updateFrameWidth();
    void        updateStyledFrameWidths();

    QRect       frect;
    int         frameStyle;
    short       lineWidth;
    short       midLineWidth;
    short       frameWidth;
    short       leftFrameWidth, rightFrameWidth;
    short       topFrameWidth, bottomFrameWidth;

    inline void init();

};

QT_END_NAMESPACE

#endif // QFRAME_P_H
