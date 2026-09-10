include_guard(GLOBAL)

if(NOT TARGET GeographicLib::GeographicLib)
    if(NOT COMMAND CPMAddPackage)
        include("${CMAKE_CURRENT_LIST_DIR}/../../../cmake/modules/CPM.cmake")
    endif()
    CPMAddPackage(
        NAME geographiclib
        VERSION 2.7
        GITHUB_REPOSITORY geographiclib/geographiclib
        GIT_TAG r2.7
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
endif()
