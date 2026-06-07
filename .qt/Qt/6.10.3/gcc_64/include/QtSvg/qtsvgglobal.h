// Copyright (C) 2016 Intel Corporation.
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTSVGGLOBAL_H
#define QTSVGGLOBAL_H

#include <QtCore/qglobal.h>
#include <QtSvg/qtsvgexports.h>

QT_BEGIN_NAMESPACE

namespace QtSvg {

enum Option : quint32 {
    NoOption           = 0x00,
    Tiny12FeaturesOnly = 0x01,
    AssumeTrustedSource = 0x02,
    // reserved for non-animation options: 0x04
    // reserved for non-animation options: 0x08
    DisableSMILAnimations = 0x10,
    DisableCSSAnimations = 0x20,
    // reserved for potentially other animations: 0x40
    // reserved for potentially other animations: 0x80
    DisableAnimations = 0xf0,
    // next value for non-animations: 0x0100
};
Q_DECLARE_FLAGS(Options, Option)
Q_DECLARE_OPERATORS_FOR_FLAGS(Options)

}

QT_END_NAMESPACE

#endif
