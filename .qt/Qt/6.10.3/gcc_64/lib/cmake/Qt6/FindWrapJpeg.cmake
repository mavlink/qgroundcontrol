# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

include(QtFindWrapHelper NO_POLICY_SCOPE)

qt_find_package_system_or_bundled(wrap_jpeg
    FRIENDLY_PACKAGE_NAME "Jpeg"
    WRAP_PACKAGE_TARGET "WrapJpeg::WrapJpeg"
    WRAP_PACKAGE_FOUND_VAR_NAME "WrapJpeg_FOUND"
    BUNDLED_PACKAGE_NAME "BundledLibjpeg"
    BUNDLED_PACKAGE_TARGET "BundledLibjpeg"
    SYSTEM_PACKAGE_NAME "WrapSystemJpeg"
    SYSTEM_PACKAGE_TARGET "WrapSystemJpeg::WrapSystemJpeg"
)
