# ============================================================================
# CreateAppImage.cmake
# Creates AppImage packages for Linux distribution
# ============================================================================
# cmake-lint: disable=C0103
# cmake-lint: disable=C0301
# cmake-lint: disable=E1126
#
# TODO: Implement go-appimage and signing
#

message(STATUS "QGC: Creating AppImage...")

# During install(SCRIPT), CMAKE_BINARY_DIR may not refer to the original build
# tree. QGC_BUILD_DIR is passed explicitly from Install.cmake to ensure we
# always reference the correct build directory.
if(DEFINED QGC_BUILD_DIR AND NOT QGC_BUILD_DIR STREQUAL "")
    set(QGC_APPIMAGE_BUILD_DIR "${QGC_BUILD_DIR}")
elseif(DEFINED CMAKE_BINARY_DIR AND NOT CMAKE_BINARY_DIR STREQUAL "")
    set(QGC_APPIMAGE_BUILD_DIR "${CMAKE_BINARY_DIR}")
else()
    message(FATAL_ERROR "QGC: Cannot determine build directory for AppImage creation. "
                        "Neither QGC_BUILD_DIR nor CMAKE_BINARY_DIR is set."
    )
endif()

# Detect AppDir layout: plain install prefix, usr-prefixed, or legacy AppDir
if(EXISTS "${CMAKE_INSTALL_PREFIX}/usr/bin/${CMAKE_PROJECT_NAME}")
    set(APPDIR_PATH "${CMAKE_INSTALL_PREFIX}")
    set(APPDIR_USR_PATH "${CMAKE_INSTALL_PREFIX}/usr")
elseif(EXISTS "${QGC_APPIMAGE_BUILD_DIR}/AppDir/usr/bin/${CMAKE_PROJECT_NAME}")
    set(APPDIR_PATH "${QGC_APPIMAGE_BUILD_DIR}/AppDir")
    set(APPDIR_USR_PATH "${APPDIR_PATH}/usr")
else()
    message(FATAL_ERROR "QGC: Could not locate staged AppDir for ${CMAKE_PROJECT_NAME}. "
                        "Checked ${CMAKE_INSTALL_PREFIX}/usr/bin " "and ${QGC_APPIMAGE_BUILD_DIR}/AppDir/usr/bin."
    )
endif()

set(APPIMAGE_PATH "${QGC_APPIMAGE_BUILD_DIR}/${CMAKE_PROJECT_NAME}-${CMAKE_SYSTEM_PROCESSOR}.AppImage")

# ============================================================================
# Helper Functions
# ============================================================================

include("${CMAKE_CURRENT_LIST_DIR}/../modules/Download.cmake")

# Usage: download_tool(var url [EXPECTED_HASH hash])
function(download_tool var url)
    cmake_parse_arguments(dt "" "EXPECTED_HASH" "" ${ARGN})
    cmake_path(GET url FILENAME _name)
    set(args
        FILENAME
        "${_name}"
        DESTINATION_DIR
        "${QGC_APPIMAGE_BUILD_DIR}/tools"
        RESULT_VAR
        _dest
        URLS
        "${url}"
        TIMEOUT
        300
        INACTIVITY_TIMEOUT
        30
        LOG_TAG
        "QGC"
    )
    if(dt_EXPECTED_HASH)
        list(APPEND args EXPECTED_HASH "${dt_EXPECTED_HASH}")
    endif()
    qgc_resilient_download(${args})
    file(
        CHMOD
        "${_dest}"
        FILE_PERMISSIONS
        OWNER_READ
        OWNER_WRITE
        OWNER_EXECUTE
        GROUP_READ
        GROUP_EXECUTE
        WORLD_READ
        WORLD_EXECUTE
    )
    # Dynamic output name is part of the helper's public contract.
    set(${var}_PATH
        "${_dest}"
        PARENT_SCOPE
    )
endfunction()

# ============================================================================
# Download Required Tools
# ============================================================================

message(STATUS "QGC: Downloading AppImage build tools...")

if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
    set(_linuxdeploy_hash SHA256=c20cd71e3a4e3b80c3483cef793cda3f4e990aca14014d23c544ca3ce1270b4d)
    set(_appimagetool_hash SHA256=ed4ce84f0d9caff66f50bcca6ff6f35aae54ce8135408b3fa33abfc3cb384eb0)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    set(_linuxdeploy_hash SHA256=620095110d693282b8ebeb244a95b5e911cf8f65f76c88b4b47d16ae6346fcff)
    set(_appimagetool_hash SHA256=f0837e7448a0c1e4e650a93bb3e85802546e60654ef287576f46c71c126a9158)
endif()

download_tool(
    LINUXDEPLOY
    https://github.com/linuxdeploy/linuxdeploy/releases/download/1-alpha-20251107-1/linuxdeploy-${CMAKE_SYSTEM_PROCESSOR}.AppImage
    EXPECTED_HASH
    "${_linuxdeploy_hash}"
)
download_tool(
    APPIMAGETOOL
    https://github.com/AppImage/appimagetool/releases/download/1.9.1/appimagetool-${CMAKE_SYSTEM_PROCESSOR}.AppImage
    EXPECTED_HASH "${_appimagetool_hash}"
)

# x86_64-only, rolling "continuous" release (no hash pin); skipped when
# QGC_RUN_APPIMAGELINT is OFF (Fedora/Arch, where the compat report is noise).
if(QGC_RUN_APPIMAGELINT AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
    download_tool(
        APPIMAGELINT
        https://github.com/TheAssassin/appimagelint/releases/download/continuous/appimagelint-${CMAKE_SYSTEM_PROCESSOR}.AppImage
    )
endif()

# ============================================================================
# Bundle Runtime Dependencies
# ============================================================================

message(STATUS "QGC: Bundling runtime dependencies with linuxdeploy...")

# linuxdeploy can fail to resolve libblas.so.3 on runners where alternatives
# are temporarily inconsistent after cached apt restores.
set(_linuxdeploy_extra_args)
execute_process(
    COMMAND /bin/sh -c "ldconfig -p | awk '/libblas\\.so\\.3/{print $NF; exit}'"
    OUTPUT_VARIABLE _blas_path
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
if(NOT _blas_path)
    # Debian/Ubuntu keep libs under /usr/lib/<multiarch>/; Fedora/Arch/openSUSE
    # use /usr/lib64/ (and Arch has a flat /usr/lib/). Cover all layouts.
    file(
        GLOB
        _blas_candidates
        "/usr/lib/*/libblas.so.3"
        "/usr/lib/*/openblas*/libblas.so.3"
        "/usr/lib64/libblas.so.3"
        "/usr/lib64/openblas*/libblas.so.3"
        "/usr/lib/libblas.so.3"
        "/lib/*/libblas.so.3"
        "/lib64/libblas.so.3"
    )
    list(LENGTH _blas_candidates _blas_candidate_count)
    if(_blas_candidate_count GREATER 0)
        list(GET _blas_candidates 0 _blas_path)
    endif()
endif()

if(_blas_path AND EXISTS "${_blas_path}")
    message(STATUS "QGC: Using BLAS runtime dependency: ${_blas_path}")
    list(APPEND _linuxdeploy_extra_args --library "${_blas_path}")
else()
    message(WARNING "QGC: Could not pre-resolve libblas.so.3; linuxdeploy will attempt auto-resolution")
endif()

# linuxdeploy's bundled strip aborts on newer toolchains (Fedora/Arch); keep
# stripping only on the Debian/Ubuntu portable floor where it works.
if(NOT QGC_LINUX_DISTRO_FAMILY STREQUAL "debian")
    set(ENV{NO_STRIP} 1)
endif()

# Samba's private libs live in a non-ldconfig dir, so expose them on
# LD_LIBRARY_PATH for linuxdeploy; the globs are no-ops where samba is absent.
file(GLOB _private_lib_dirs "/usr/lib64/samba" "/usr/lib/samba" "/usr/lib/*/samba")
foreach(dir IN LISTS _private_lib_dirs)
    set(ENV{LD_LIBRARY_PATH} "${dir}:$ENV{LD_LIBRARY_PATH}")
endforeach()

execute_process(
    COMMAND
        "${LINUXDEPLOY_PATH}" --appdir "${APPDIR_PATH}" --executable "${APPDIR_USR_PATH}/bin/${CMAKE_PROJECT_NAME}"
        --desktop-file "${APPDIR_USR_PATH}/share/applications/${QGC_PACKAGE_NAME}.desktop" --custom-apprun
        "${QGC_APPIMAGE_BUILD_DIR}/AppRun" --icon-file
        "${APPDIR_USR_PATH}/share/icons/hicolor/256x256/apps/${CMAKE_PROJECT_NAME}.png" ${_linuxdeploy_extra_args}
        COMMAND_ECHO STDOUT COMMAND_ERROR_IS_FATAL ANY
)

# ============================================================================
# Build Final AppImage
# ============================================================================

message(STATUS "QGC: Building AppImage package...")

set(ENV{ARCH} ${CMAKE_SYSTEM_PROCESSOR})
set(ENV{VERSION} ${CMAKE_PROJECT_VERSION})

# --no-appstream: appimagetool's AppStream validation fetches remote URLs (e.g.
# screenshot links), making packaging fail on unrelated web changes. The metainfo
# is already validated offline (appstreamcli validate --no-net) in Install.cmake.
set(_appimagetool_args --no-appstream)
if(QGC_APPIMAGE_UPDATE_INFORMATION)
    list(APPEND _appimagetool_args --updateinformation "${QGC_APPIMAGE_UPDATE_INFORMATION}")
endif()
execute_process(COMMAND "${APPIMAGETOOL_PATH}" ${_appimagetool_args} "${APPDIR_PATH}" "${APPIMAGE_PATH}" COMMAND_ECHO
                        STDOUT COMMAND_ERROR_IS_FATAL ANY
)

if(QGC_APPIMAGE_UPDATE_INFORMATION AND NOT EXISTS "${APPIMAGE_PATH}.zsync")
    message(FATAL_ERROR "QGC: appimagetool did not create the required update file: ${APPIMAGE_PATH}.zsync")
endif()

message(STATUS "QGC: AppImage created successfully: ${APPIMAGE_PATH}")

# ============================================================================
# Validation & Linting
# ============================================================================

if(NOT QGC_RUN_APPIMAGELINT)
    message(STATUS "QGC: AppImageLint disabled (QGC_RUN_APPIMAGELINT=OFF), skipping validation")
elseif(EXISTS "${APPIMAGELINT_PATH}")
    message(STATUS "QGC: Running AppImage linter...")
    set(APPIMAGELINT_REPORT_PATH "${QGC_APPIMAGE_BUILD_DIR}/appimagelint-report.json")
    file(REMOVE "${APPIMAGELINT_REPORT_PATH}")
    # Packaging tools use extract-and-run in containers, but appimagelint's
    # helper runtime must mount the target AppImage through the provided FUSE
    # device. Inheriting this variable makes that helper extract and exit.
    execute_process(COMMAND "${CMAKE_COMMAND}" -E env --unset=APPIMAGE_EXTRACT_AND_RUN "${APPIMAGELINT_PATH}"
                            --json-report "${APPIMAGELINT_REPORT_PATH}" "${APPIMAGE_PATH}"
                    RESULT_VARIABLE LINT_RESULT COMMAND_ECHO STDOUT
    )
    if(NOT LINT_RESULT EQUAL 0)
        message(WARNING "QGC: AppImageLint failed to complete - see output above")
    elseif(NOT EXISTS "${APPIMAGELINT_REPORT_PATH}")
        message(WARNING "QGC: AppImageLint completed without producing its JSON report")
    else()
        message(STATUS "QGC: AppImageLint completed; review findings above and in ${APPIMAGELINT_REPORT_PATH}")
    endif()
else()
    message(STATUS "QGC: AppImageLint not available, skipping validation")
endif()
