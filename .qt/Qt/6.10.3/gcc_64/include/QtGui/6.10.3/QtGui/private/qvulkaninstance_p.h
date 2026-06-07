// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QVULKANINSTANCE_P_H
#define QVULKANINSTANCE_P_H

#include <QtGui/private/qtguiglobal_p.h>

#if QT_CONFIG(vulkan) || defined(Q_QDOC)

#include "qvulkaninstance.h"
#include <private/qvulkanfunctions_p.h>
#include <QtCore/QHash>

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

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QVulkanInstancePrivate
{
public:
    QVulkanInstancePrivate(QVulkanInstance *q)
        : q_ptr(q),
          vkInst(VK_NULL_HANDLE),
          errorCode(VK_SUCCESS)
    { }
    ~QVulkanInstancePrivate() { reset(); }
    static QVulkanInstancePrivate *get(QVulkanInstance *q) { return q->d_ptr.data(); }

    bool ensureVulkan();
    void reset();

    QVulkanInstance *q_ptr;
    QScopedPointer<QPlatformVulkanInstance> platformInst;
    VkInstance vkInst;
    QVulkanInstance::Flags flags;
    QByteArrayList layers;
    QByteArrayList extensions;
    QVersionNumber apiVersion;
    VkResult errorCode;
    QScopedPointer<QVulkanFunctions> funcs;
    QHash<VkDevice, QVulkanDeviceFunctions *> deviceFuncs;
    QList<QVulkanInstance::DebugFilter> debugFilters; // legacy filters based on VK_EXT_debug_report
    QList<QVulkanInstance::DebugUtilsFilter> debugUtilsFilters; // the modern version based on VK_EXT_debug_utils
};

QT_END_NAMESPACE

#endif // QT_CONFIG(vulkan)

#endif // QVULKANINSTANCE_P_H
