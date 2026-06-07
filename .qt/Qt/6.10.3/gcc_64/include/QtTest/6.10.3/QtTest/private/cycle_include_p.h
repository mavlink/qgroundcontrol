// Copyright (C) 2025 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QBENCHLIB_CYCLE_INCLUDE_H
#define QBENCHLIB_CYCLE_INCLUDE_H

#include <QtCore/qcompilerdetection.h>

// This file suppresses compilation warnings coming from cycle_p.h.
// Include this file instead of cycle_p.h directly.

#define QBENCHLIB_INCLUDING_CYCLE_P
QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wundef")
QT_WARNING_DISABLE_GCC("-Wundef")
#include "cycle_p.h"
QT_WARNING_POP

#endif // QBENCHLIB_CYCLE_INCLUDE_H
