// Copyright (C) 2025 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QLOGGINGCATEGORY_P_H
#define QLOGGINGCATEGORY_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of a number of Qt sources files.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qloggingcategory.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

// enabledDebug = enabledWarning = enabledCritical = enableInfo = true;
static constexpr int DefaultLoggingCategoryEnabledValue = 0x01010101;

constexpr inline
QLoggingCategory::QLoggingCategory(UnregisteredInitialization, const char *category) noexcept
    : name(category),
      enabled(DefaultLoggingCategoryEnabledValue),
      placeholder{}
{}

QT_END_NAMESPACE

#endif // QLOGGINGCATEGORY_P_H
