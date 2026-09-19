include_guard(GLOBAL)

if(TARGET GeographicLib::GeographicLib)
    return()
endif()

if(NOT COMMAND CPMAddPackage)
    include("${CMAKE_CURRENT_LIST_DIR}/../../../cmake/modules/CPM.cmake")
endif()
include(GNUInstallDirs)
include("${CMAKE_CURRENT_LIST_DIR}/../../../cmake/modules/CPMPatchCache.cmake")
qgc_cpm_patch_cache_key(
    _geographiclib_cache_key
    GIT_REPOSITORY
    "https://github.com/geographiclib/geographiclib.git"
    GIT_TAG
    r2.7
    PATCHES
    "${CMAKE_CURRENT_LIST_DIR}/geographiclib.patch"
)

CPMAddPackage(
    NAME geographiclib
    VERSION 2.7
    GITHUB_REPOSITORY geographiclib/geographiclib
    GIT_TAG r2.7
    CUSTOM_CACHE_KEY "${_geographiclib_cache_key}"
    # System package installs its config as `GeographicLib`, not `geographiclib`.
    FIND_PACKAGE_ARGUMENTS "NAMES GeographicLib"
    OPTIONS "BUILD_BOTH_LIBS OFF"
            "BUILD_DOCUMENTATION OFF"
            "BUILD_MANPAGES OFF"
            "PACKAGE_DEBUG_LIBS OFF"
            "APPLE_MULTIPLE_ARCHITECTURES OFF"
            "INCDIR OFF"
            "BINDIR OFF"
            "SBINDIR OFF"
            "LIBDIR ${CMAKE_INSTALL_LIBDIR}"
            "DLLDIR ${CMAKE_INSTALL_BINDIR}"
            "MANDIR OFF"
            "CMAKEDIR OFF"
            "PKGDIR OFF"
            "DOCDIR OFF"
            "EXAMPLEDIR OFF"
    PATCHES "${CMAKE_CURRENT_LIST_DIR}/geographiclib.patch"
)

if(NOT TARGET GeographicLib::GeographicLib)
    message(FATAL_ERROR "QGC: geographiclib (required dependency) was not resolved")
endif()

if(TARGET GeographicLib_STATIC AND COMMAND qgc_disable_dependency_warnings)
    qgc_disable_dependency_warnings(GeographicLib_STATIC)
endif()
