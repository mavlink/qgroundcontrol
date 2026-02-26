# ============================================================================
# CreateAppImage.cmake
# Creates AppImage packages for Linux distribution
# ============================================================================
#
# TODO: Implement go-appimage, update information with GitHub Releases, signing
#

message(STATUS "QGC: Creating AppImage...")

set(APPDIR_PATH "${CMAKE_BINARY_DIR}/AppDir")
set(APPIMAGE_PATH "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}-${CMAKE_SYSTEM_PROCESSOR}.AppImage")

# ============================================================================
# Helper Functions
# ============================================================================

# Download and cache build tools
# Usage: download_tool(VAR URL [EXPECTED_HASH hash])
function(download_tool VAR URL)
    cmake_parse_arguments(_DT "" "EXPECTED_HASH" "" ${ARGN})
    cmake_path(GET URL FILENAME _name)
    set(_dest "${CMAKE_BINARY_DIR}/tools/${_name}")
    if(NOT EXISTS "${_dest}")
        file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/tools")
        message(STATUS "QGC: Downloading ${_name} to ${_dest}")
        set(_download_args
            DOWNLOAD "${URL}" "${_dest}"
            STATUS _status
            TLS_VERIFY ON
            INACTIVITY_TIMEOUT 30
            TIMEOUT 300
        )
        if(_DT_EXPECTED_HASH)
            list(APPEND _download_args EXPECTED_HASH "${_DT_EXPECTED_HASH}")
        endif()
        set(_max_attempts 3)
        set(_attempt 1)
        while(_attempt LESS_EQUAL _max_attempts)
            file(${_download_args})
            list(GET _status 0 _result)
            if(_result EQUAL 0)
                break()
            endif()
            if(_attempt EQUAL _max_attempts)
                message(FATAL_ERROR "Failed to download ${URL} to ${_dest}: ${_status}")
            endif()
            message(WARNING "QGC: Download attempt ${_attempt}/${_max_attempts} failed for ${_name}: ${_status}. Retrying...")
            # Remove partial download before retry
            file(REMOVE "${_dest}")
            math(EXPR _attempt "${_attempt} + 1")
        endwhile()
        file(CHMOD "${_dest}" FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    endif()
    set(${VAR}_PATH "${_dest}" PARENT_SCOPE)
endfunction()

# ============================================================================
# Download Required Tools
# ============================================================================

message(STATUS "QGC: Downloading AppImage build tools...")

if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
    set(_LINUXDEPLOY_HASH SHA256=c20cd71e3a4e3b80c3483cef793cda3f4e990aca14014d23c544ca3ce1270b4d)
    set(_APPIMAGETOOL_HASH SHA256=ed4ce84f0d9caff66f50bcca6ff6f35aae54ce8135408b3fa33abfc3cb384eb0)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    set(_LINUXDEPLOY_HASH SHA256=620095110d693282b8ebeb244a95b5e911cf8f65f76c88b4b47d16ae6346fcff)
    set(_APPIMAGETOOL_HASH SHA256=f0837e7448a0c1e4e650a93bb3e85802546e60654ef287576f46c71c126a9158)
endif()

download_tool(LINUXDEPLOY https://github.com/linuxdeploy/linuxdeploy/releases/download/1-alpha-20251107-1/linuxdeploy-${CMAKE_SYSTEM_PROCESSOR}.AppImage
    EXPECTED_HASH "${_LINUXDEPLOY_HASH}")
download_tool(APPIMAGETOOL https://github.com/AppImage/appimagetool/releases/download/1.9.1/appimagetool-${CMAKE_SYSTEM_PROCESSOR}.AppImage
    EXPECTED_HASH "${_APPIMAGETOOL_HASH}")

# AppImageLint is only available for x86_64; uses a rolling "continuous" release so no hash pin
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
    download_tool(APPIMAGELINT https://github.com/TheAssassin/appimagelint/releases/download/continuous/appimagelint-${CMAKE_SYSTEM_PROCESSOR}.AppImage)
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
    file(GLOB _blas_candidates
        "/usr/lib/*/libblas.so.3"
        "/usr/lib/*/openblas*/libblas.so.3"
        "/lib/*/libblas.so.3"
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

execute_process(
    COMMAND "${LINUXDEPLOY_PATH}"
            --appdir "${APPDIR_PATH}"
            --executable "${APPDIR_PATH}/usr/bin/${CMAKE_PROJECT_NAME}"
            --desktop-file "${APPDIR_PATH}/usr/share/applications/${QGC_PACKAGE_NAME}.desktop"
            --custom-apprun "${CMAKE_BINARY_DIR}/AppRun"
            --icon-file "${APPDIR_PATH}/usr/share/icons/hicolor/256x256/apps/${CMAKE_PROJECT_NAME}.png"
            ${_linuxdeploy_extra_args}
    COMMAND_ECHO STDOUT
    COMMAND_ERROR_IS_FATAL ANY
)

# ============================================================================
# Build Final AppImage
# ============================================================================

message(STATUS "QGC: Building AppImage package...")

set(ENV{ARCH} ${CMAKE_SYSTEM_PROCESSOR})
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
