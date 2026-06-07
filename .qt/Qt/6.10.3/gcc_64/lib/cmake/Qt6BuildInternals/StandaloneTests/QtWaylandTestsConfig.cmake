# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

set(__standalone_parts_qt_packages_args
    QT_MODULE_PACKAGES
        WaylandCompositor
        WaylandClientFeaturesPrivate
        WaylandEglCompositorHwIntegrationPrivate
        WaylandCompositorXdgShell
        WaylandCompositorIviapplication
        WaylandCompositorWLShell
        WaylandCompositorPresentationTime
)
qt_internal_find_standalone_parts_qt_packages(__standalone_parts_qt_packages_args)
