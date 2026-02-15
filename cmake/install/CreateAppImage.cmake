# ============================================================================
# CreateAppImage.cmake
# Creates AppImage packages for Linux distribution
# ============================================================================
#
# TODO: Implement go-appimage, update information with GitHub Releases, signing
#

message(STATUS "QGC: Creating AppImage...")

set(APPDIR_PATH "${CMAKE_BINARY_DIR}/AppDir")

string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" _qgc_system_processor)
if(_qgc_system_processor STREQUAL "x86_64" OR _qgc_system_processor STREQUAL "amd64")
    set(_qgc_appimage_arch "x86_64")
elseif(_qgc_system_processor STREQUAL "aarch64" OR _qgc_system_processor STREQUAL "arm64")
    set(_qgc_appimage_arch "aarch64")
elseif(_qgc_system_processor STREQUAL "armhf" OR _qgc_system_processor STREQUAL "armv7l")
    set(_qgc_appimage_arch "armhf")
elseif(_qgc_system_processor STREQUAL "i686" OR _qgc_system_processor STREQUAL "i386")
    set(_qgc_appimage_arch "i686")
else()
    message(FATAL_ERROR "QGC: Unsupported AppImage architecture '${CMAKE_SYSTEM_PROCESSOR}'.")
endif()

set(APPIMAGE_PATH "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}-${_qgc_appimage_arch}.AppImage")
message(STATUS "QGC: AppImage architecture resolved to '${_qgc_appimage_arch}' from '${CMAKE_SYSTEM_PROCESSOR}'")

# ============================================================================
# Helper Functions
# ============================================================================

# Download and cache build tools
function(download_tool VAR URL)
    cmake_path(GET URL FILENAME _name)
    set(_dest "${CMAKE_BINARY_DIR}/tools/${_name}")
    if(NOT EXISTS "${_dest}")
        file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/tools")
        message(STATUS "QGC: Downloading ${_name} to ${_dest}")
        set(_download_ok FALSE)
        set(_status "")
        foreach(_attempt RANGE 1 3)
            file(DOWNLOAD "${URL}" "${_dest}" STATUS _status TLS_VERIFY ON)
            list(GET _status 0 _result)
            if(_result EQUAL 0)
                set(_download_ok TRUE)
                break()
            endif()

            # Avoid reusing a partial download on the next retry.
            file(REMOVE "${_dest}")
            if(_attempt LESS 3)
                message(WARNING "QGC: Download attempt ${_attempt}/3 failed for ${_name}: ${_status}")
            endif()
        endforeach()
        if(NOT _download_ok)
            message(FATAL_ERROR "Failed to download ${URL} to ${_dest} after 3 attempts: ${_status}")
        endif()
        file(CHMOD "${_dest}" FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    endif()
    set(${VAR}_PATH "${_dest}" PARENT_SCOPE)
endfunction()

# ============================================================================
# Download Required Tools
# ============================================================================

message(STATUS "QGC: Downloading AppImage build tools...")

download_tool(LINUXDEPLOY https://github.com/linuxdeploy/linuxdeploy/releases/download/1-alpha-20251107-1/linuxdeploy-${_qgc_appimage_arch}.AppImage)
download_tool(APPIMAGETOOL https://github.com/AppImage/appimagetool/releases/download/1.9.1/appimagetool-${_qgc_appimage_arch}.AppImage)

# AppImageLint is only available for x86_64
if(_qgc_appimage_arch STREQUAL "x86_64")
    download_tool(APPIMAGELINT https://github.com/TheAssassin/appimagelint/releases/download/continuous/appimagelint-${_qgc_appimage_arch}.AppImage)
endif()

# ============================================================================
# Bundle Runtime Dependencies
# ============================================================================

message(STATUS "QGC: Bundling runtime dependencies with linuxdeploy...")

# Hosted CI runners often do not support FUSE mounts for AppImage binaries.
set(ENV{APPIMAGE_EXTRACT_AND_RUN} 1)

execute_process(
    COMMAND "${LINUXDEPLOY_PATH}"
            --appdir "${APPDIR_PATH}"
            --executable "${APPDIR_PATH}/usr/bin/${CMAKE_PROJECT_NAME}"
            --desktop-file "${APPDIR_PATH}/usr/share/applications/${QGC_PACKAGE_NAME}.desktop"
            --custom-apprun "${CMAKE_BINARY_DIR}/AppRun"
            --icon-file "${APPDIR_PATH}/usr/share/icons/hicolor/256x256/apps/${CMAKE_PROJECT_NAME}.png"
    COMMAND_ECHO STDOUT
    COMMAND_ERROR_IS_FATAL ANY
)

# ============================================================================
# Build Final AppImage
# ============================================================================

message(STATUS "QGC: Building AppImage package...")

set(ENV{ARCH} ${_qgc_appimage_arch})
set(ENV{VERSION} ${CMAKE_PROJECT_VERSION})

execute_process(
    COMMAND "${APPIMAGETOOL_PATH}" "${APPDIR_PATH}" "${APPIMAGE_PATH}"
    COMMAND_ECHO STDOUT
    COMMAND_ERROR_IS_FATAL ANY
)

message(STATUS "QGC: AppImage created successfully: ${APPIMAGE_PATH}")

# ============================================================================
# Validation & Linting
# ============================================================================

if(EXISTS "${APPIMAGELINT_PATH}")
    message(STATUS "QGC: Running AppImage linter...")
    execute_process(
        COMMAND "${APPIMAGELINT_PATH}" "${APPIMAGE_PATH}"
        RESULT_VARIABLE LINT_RESULT
        COMMAND_ECHO STDOUT
    )
    if(NOT LINT_RESULT EQUAL 0)
        message(WARNING "QGC: AppImageLint reported issues - see output above")
    else()
        message(STATUS "QGC: AppImage passed validation")
    endif()
else()
    message(STATUS "QGC: AppImageLint not available, skipping validation")
endif()
