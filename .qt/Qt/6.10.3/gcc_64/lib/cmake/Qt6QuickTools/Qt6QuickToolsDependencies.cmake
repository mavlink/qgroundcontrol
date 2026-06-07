# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Find "ModuleTools" dependencies, which are other ModuleTools packages.
set(Qt6QuickTools_FOUND TRUE)

set(__qt_QuickTools_tool_third_party_deps "")
if(__qt_QuickTools_tool_third_party_deps)
    _qt_internal_find_third_party_dependencies("QuickTools" __qt_QuickTools_tool_third_party_deps)
endif()

set(__qt_QuickTools_tool_deps "Qt6QmlTools\;6.10.3")
if(__qt_QuickTools_tool_deps)
    _qt_internal_find_tool_dependencies("QuickTools" __qt_QuickTools_tool_deps)
endif()
