// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSCROLLBAR_P_H
#define QSCROLLBAR_P_H

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
#include "qscrollbar.h"
#include "private/qabstractslider_p.h"
#include "qstyle.h"

#include <QtCore/qbasictimer.h>

QT_REQUIRE_CONFIG(scrollbar);

QT_BEGIN_NAMESPACE

class QScrollBarPrivate : public QAbstractSliderPrivate
{
    Q_DECLARE_PUBLIC(QScrollBar)
public:
    QStyle::SubControl pressedControl;
    bool pointerOutsidePressedControl;

    int clickOffset, snapBackPosition;

    void activateControl(uint control, int threshold = 500);
    void stopRepeatAction();
    int pixelPosToRangeValue(int pos) const;
    void init();
    bool updateHoverControl(const QPoint &pos);
    QStyle::SubControl newHoverControl(const QPoint &pos);

    QStyle::SubControl hoverControl;
    QRect hoverRect;

    bool transient;
    void setTransient(bool value);

    bool flashed;
    QBasicTimer flashTimer;
    void flash();
};

QT_END_NAMESPACE

#endif // QSCROLLBAR_P_H
