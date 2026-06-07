// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWAYLANDVULKANWINDOW_P_H
#define QWAYLANDVULKANWINDOW_P_H

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

#include "qwaylandwindow_p.h"
#include "qwaylandvulkaninstance_p.h"

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandVulkanWindow : public QWaylandWindow
{
public:
    explicit QWaylandVulkanWindow(QWindow *window, QWaylandDisplay *display);
    ~QWaylandVulkanWindow() override;

    WindowType windowType() const override;
    void invalidateSurface() override;

    VkSurfaceKHR *vkSurface();

private:
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
};

} // namespace QtWaylandClient

QT_END_NAMESPACE

#endif // QWAYLANDVULKANWINDOW_P_H
