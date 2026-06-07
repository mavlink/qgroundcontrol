# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Find the Slog2 library

# Will make the target Slog2::Slog2 available when found.
if(TARGET Slog2::Slog2)
    set(Slog2_FOUND TRUE)
    return()
endif()

find_library(Slog2_LIBRARY NAMES "slog2")
find_path(Slog2_INCLUDE_DIR NAMES "sys/slog2.h" DOC "The Slog2 Include path")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Slog2 DEFAULT_MSG Slog2_INCLUDE_DIR Slog2_LIBRARY)

mark_as_advanced(Slog2_INCLUDE_DIR Slog2_LIBRARY)

if(Slog2_FOUND)
    add_library(Slog2::Slog2 INTERFACE IMPORTED)
    target_link_libraries(Slog2::Slog2 INTERFACE ${Slog2_LIBRARY})
    target_include_directories(Slog2::Slog2 INTERFACE ${Slog2_INCLUDE_DIR})
endif()
