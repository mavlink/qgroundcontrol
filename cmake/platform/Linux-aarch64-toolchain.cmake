# ----------------------------------------------------------------------------
# Cross-compile QGroundControl for Linux aarch64 from an x86_64 host.
#
# Pairs with:
#   - deploy/docker/Dockerfile (target: linux-cross)            (full toolchain image)
#   - deploy/docker/install-sysroot-aarch64.sh                  (multiarch :arm64 sysroot)
#
# Required cache vars (pass via -D on the initial CMake invocation):
#   QT_HOST_PATH    Host Qt (matching version) for moc/rcc/uic during AUTOMOC
#   CMAKE_PREFIX_PATH ⊇ <target Qt>/lib/cmake
#
# Optional cache vars / env:
#   QGC_AARCH64_SYSROOT  Defaults to $ENV{SYSROOT} or /opt/sysroot.
#
# Typical invocation (inside Docker image):
#   "${QT_HOST_PATH}/bin/qt-cmake" \
#       -S /project/source -B /project/build -G Ninja \
#       -DCMAKE_TOOLCHAIN_FILE=/project/source/cmake/platform/Linux-aarch64-toolchain.cmake \
#       -DCMAKE_PREFIX_PATH=/opt/Qt/${QT_VERSION}/gcc_arm64 \
#       -DQT_HOST_PATH=/opt/Qt/${QT_VERSION}/gcc_64
# ----------------------------------------------------------------------------

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# Sysroot resolution: cache var → env → /opt/sysroot. Validated below.
if(NOT DEFINED QGC_AARCH64_SYSROOT)
    if(DEFINED ENV{SYSROOT})
        set(QGC_AARCH64_SYSROOT "$ENV{SYSROOT}")
    else()
        set(QGC_AARCH64_SYSROOT "/opt/sysroot")
    endif()
endif()
set(QGC_AARCH64_SYSROOT "${QGC_AARCH64_SYSROOT}" CACHE PATH "aarch64 sysroot for cross-build")

if(NOT IS_DIRECTORY "${QGC_AARCH64_SYSROOT}")
    message(FATAL_ERROR
        "QGC_AARCH64_SYSROOT='${QGC_AARCH64_SYSROOT}' is not a directory. "
        "Run deploy/docker/install-sysroot-aarch64.sh or set -DQGC_AARCH64_SYSROOT=<path>.")
endif()

set(CMAKE_SYSROOT "${QGC_AARCH64_SYSROOT}")

# Cross toolchain — apt: gcc-aarch64-linux-gnu g++-aarch64-linux-gnu.
set(_qgc_cross_prefix aarch64-linux-gnu)
set(CMAKE_C_COMPILER   ${_qgc_cross_prefix}-gcc)
set(CMAKE_CXX_COMPILER ${_qgc_cross_prefix}-g++)
set(CMAKE_AR           ${_qgc_cross_prefix}-ar)
set(CMAKE_RANLIB       ${_qgc_cross_prefix}-ranlib)
set(CMAKE_STRIP        ${_qgc_cross_prefix}-strip)
set(CMAKE_NM           ${_qgc_cross_prefix}-nm)
set(CMAKE_OBJDUMP      ${_qgc_cross_prefix}-objdump)
set(CMAKE_OBJCOPY      ${_qgc_cross_prefix}-objcopy)

# Search libs/headers/packages inside the sysroot; programs from the host PATH
# (cmake/git/python/aqt's host moc handled separately via QT_HOST_PATH).
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# pkg-config must read .pc files from the sysroot, not the host (host .pc files
# carry x86_64 -L/-I paths that silently link host libraries into the target).
find_program(_qgc_cross_pkgconfig ${_qgc_cross_prefix}-pkg-config)
if(_qgc_cross_pkgconfig)
    set(PKG_CONFIG_EXECUTABLE "${_qgc_cross_pkgconfig}" CACHE FILEPATH "Cross pkg-config" FORCE)
endif()
set(ENV{PKG_CONFIG_SYSROOT_DIR} "${QGC_AARCH64_SYSROOT}")
set(ENV{PKG_CONFIG_LIBDIR}
    "${QGC_AARCH64_SYSROOT}/usr/lib/${_qgc_cross_prefix}/pkgconfig:${QGC_AARCH64_SYSROOT}/usr/share/pkgconfig")

# Sysroot flags need to land in *_INIT (not CMAKE_*_FLAGS) so they reach the
# compiler-detection step before the toolchain finishes loading.
set(CMAKE_C_FLAGS_INIT          "--sysroot=${QGC_AARCH64_SYSROOT}")
set(CMAKE_CXX_FLAGS_INIT        "--sysroot=${QGC_AARCH64_SYSROOT}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "--sysroot=${QGC_AARCH64_SYSROOT}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "--sysroot=${QGC_AARCH64_SYSROOT}")

# Re-root find_package onto the target Qt prefix (MODE_PACKAGE=ONLY). Host Qt must not
# be a root or optional components (e.g. Qt6::SerialPort) resolve to the x86 .so.
set(CMAKE_FIND_ROOT_PATH "${QGC_AARCH64_SYSROOT}" ${CMAKE_PREFIX_PATH})

# Host Qt tools for AUTOMOC/AUTORCC. Target moc cannot parse host C++ headers;
# Qt6's qt-cmake handles this when invoked from host, but a raw cmake invocation
# needs the executables pinned explicitly.
if(DEFINED QT_HOST_PATH)
    foreach(_tool moc rcc uic)
        string(TOUPPER ${_tool} _TOOL_UPPER)
        if(EXISTS "${QT_HOST_PATH}/libexec/${_tool}")
            set(Qt6_${_TOOL_UPPER}_EXECUTABLE "${QT_HOST_PATH}/libexec/${_tool}"
                CACHE FILEPATH "Host ${_tool} for cross-build" FORCE)
        elseif(EXISTS "${QT_HOST_PATH}/bin/${_tool}")
            set(Qt6_${_TOOL_UPPER}_EXECUTABLE "${QT_HOST_PATH}/bin/${_tool}"
                CACHE FILEPATH "Host ${_tool} for cross-build" FORCE)
        endif()
    endforeach()
    # AUTOMOC re-runs the cross-compiler over host headers without this; the host
    # compiler-predefines that AUTOMOC scrapes don't match the cross target.
    set(CMAKE_AUTOMOC_COMPILER_PREDEFINES OFF)
endif()
