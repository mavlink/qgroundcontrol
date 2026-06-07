// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWAYLANDVULKANINSTANCE_P_H
#define QWAYLANDVULKANINSTANCE_P_H

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

#if defined(VULKAN_H_) && !defined(VK_USE_PLATFORM_WAYLAND_KHR)
#error "vulkan.h included without Wayland WSI"
#endif

#define VK_USE_PLATFORM_WAYLAND_KHR

#include <QtGui/private/qbasicvulkanplatforminstance_p.h>
#include <QLibrary>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandWindow;

class QWaylandVulkanInstance : public QBasicPlatformVulkanInstance
{
public:
    explicit QWaylandVulkanInstance(QVulkanInstance *instance);
    ~QWaylandVulkanInstance() override;

    void createOrAdoptInstance() override;
    bool supportsPresent(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, QWindow *window) override;
    void presentAboutToBeQueued(QWindow *window) override;

    VkSurfaceKHR createSurface(QWaylandWindow *window);

    void beginFrame(QWindow *window) override;
    void endFrame(QWindow *window) override;

private:
    QVulkanInstance *m_instance = nullptr;
    PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR m_getPhysDevPresSupport = nullptr;
    PFN_vkCreateWaylandSurfaceKHR m_createSurface = nullptr;
    int mFrameCallbackTimeout = 100;
};

} // namespace QtWaylandClient

QT_END_NAMESPACE

#endif // QWAYLANDVULKANINSTANCE_P_H
