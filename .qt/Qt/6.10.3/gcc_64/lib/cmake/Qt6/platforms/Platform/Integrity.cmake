# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Custom platform module file for INTEGRITY.
#
# UNIX must be set here, because this variable is cleared after the toolchain file is loaded.
#
# Once the lowest CMake version we support ships an Integrity platform module,
# we can remove this file.

set(UNIX 1)
