# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

include(QtFindWrapHelper NO_POLICY_SCOPE)

qt_find_package_system_or_bundled(wrap_freetype
    FRIENDLY_PACKAGE_NAME "Freetype"
    WRAP_PACKAGE_TARGET "WrapFreetype::WrapFreetype"
    WRAP_PACKAGE_FOUND_VAR_NAME "WrapFreetype_FOUND"
    BUNDLED_PACKAGE_NAME "BundledFreetype"
    BUNDLED_PACKAGE_TARGET "BundledFreetype"
    SYSTEM_PACKAGE_NAME "WrapSystemFreetype"
    SYSTEM_PACKAGE_TARGET "WrapSystemFreetype::WrapSystemFreetype"
)
