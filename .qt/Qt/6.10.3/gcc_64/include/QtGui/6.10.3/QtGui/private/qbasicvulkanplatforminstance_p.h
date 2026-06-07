// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QBASICVULKANPLATFORMINSTANCE_P_H
#define QBASICVULKANPLATFORMINSTANCE_P_H

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

#include <QtGui/qtguiglobal.h>

#include <QtCore/QLibrary>
#include <qpa/qplatformvulkaninstance.h>
#include <private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QBasicPlatformVulkanInstance : public QPlatformVulkanInstance
{
public:
    QBasicPlatformVulkanInstance();
    ~QBasicPlatformVulkanInstance();

    QVulkanInfoVector<QVulkanLayer> supportedLayers() const override;
    QVulkanInfoVector<QVulkanExtension> supportedExtensions() const override;
    QVersionNumber supportedApiVersion() const override;
    bool isValid() const override;
    VkResult errorCode() const override;
    VkInstance vkInstance() const override;
    QByteArrayList enabledLayers() const override;
    QByteArrayList enabledExtensions() const override;
    PFN_vkVoidFunction getInstanceProcAddr(const char *name) override;
    bool supportsPresent(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, QWindow *window) override;
    void setDebugFilters(const QList<QVulkanInstance::DebugFilter> &filters) override;
    void setDebugUtilsFilters(const QList<QVulkanInstance::DebugUtilsFilter> &filters) override;

    void destroySurface(VkSurfaceKHR surface) const;
    const QList<QVulkanInstance::DebugFilter> *debugFilters() const { return &m_debugFilters; }
    const QList<QVulkanInstance::DebugUtilsFilter> *debugUtilsFilters() const { return &m_debugUtilsFilters; }

protected:
    void loadVulkanLibrary(const QString &defaultLibraryName, int defaultLibraryVersion = -1);
    void init(QLibrary *lib);
    void initInstance(QVulkanInstance *instance, const QByteArrayList &extraExts);

    VkInstance m_vkInst = VK_NULL_HANDLE;
    PFN_vkGetInstanceProcAddr m_vkGetInstanceProcAddr = nullptr;
    PFN_vkGetPhysicalDeviceSurfaceSupportKHR m_getPhysDevSurfaceSupport;
    PFN_vkDestroySurfaceKHR m_destroySurface;

private:
    void setupDebugOutput();

    std::unique_ptr<QLibrary> m_vulkanLib;

    bool m_ownsVkInst = false;
    VkResult m_errorCode = VK_SUCCESS;
    QVulkanInfoVector<QVulkanLayer> m_supportedLayers;
    QVulkanInfoVector<QVulkanExtension> m_supportedExtensions;
    QVersionNumber m_supportedApiVersion;
    QByteArrayList m_enabledLayers;
    QByteArrayList m_enabledExtensions;

    PFN_vkCreateInstance m_vkCreateInstance;
    PFN_vkEnumerateInstanceLayerProperties m_vkEnumerateInstanceLayerProperties;
    PFN_vkEnumerateInstanceExtensionProperties m_vkEnumerateInstanceExtensionProperties;

    PFN_vkDestroyInstance m_vkDestroyInstance;

#ifdef VK_EXT_debug_utils
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
    PFN_vkDestroyDebugUtilsMessengerEXT m_vkDestroyDebugUtilsMessengerEXT;
#endif
    QList<QVulkanInstance::DebugFilter> m_debugFilters;
    QList<QVulkanInstance::DebugUtilsFilter> m_debugUtilsFilters;
};

QT_END_NAMESPACE

#endif // QBASICVULKANPLATFORMINSTANCE_P_H
