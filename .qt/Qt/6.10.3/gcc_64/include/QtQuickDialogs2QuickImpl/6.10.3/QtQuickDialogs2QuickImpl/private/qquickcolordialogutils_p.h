// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKCOLORDIALOGUTILS_P_H
#define QQUICKCOLORDIALOGUTILS_P_H


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

#include <QtCore/QtGlobal>

QT_BEGIN_NAMESPACE

std::pair<qreal, qreal> getSaturationAndValue(qreal saturation, qreal lightness);

std::pair<qreal, qreal> getSaturationAndLightness(qreal saturation, qreal value);

struct HSVA
{
    qreal h = .0;
    qreal s = .0;
    union {
        qreal v = 1.0;
        qreal l;
    };
    qreal a = 1.0;
};

QT_END_NAMESPACE

#endif // QQUICKCOLORDIALOGUTILS_P_H
