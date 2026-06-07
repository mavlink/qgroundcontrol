// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QVULKANDEFAULTINSTANCE_P_H
#define QVULKANDEFAULTINSTANCE_P_H

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

#include <QtGui/private/qtguiglobal_p.h>

#if QT_CONFIG(vulkan)

#include <QtGui/qvulkaninstance.h>
#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcGuiVk)

struct Q_GUI_EXPORT QVulkanDefaultInstance
{
    enum Flag {
        EnableValidation = 0x01
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    static Flags flags();
    static void setFlag(Flag flag, bool on = true);
    static bool hasInstance();
    static QVulkanInstance *instance();
    static void cleanup();
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QVulkanDefaultInstance::Flags)

QT_END_NAMESPACE

#endif // QT_CONFIG(vulkan)

#endif
