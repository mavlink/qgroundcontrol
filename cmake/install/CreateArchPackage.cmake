# ============================================================================
# CreateArchPackage.cmake
# Arch Linux package via makepkg (CPack has no pacman generator). Defines the
# `qgc-package` target. The AppImage remains the primary Arch deliverable, but
# an explicitly requested package target must fail if makepkg cannot build it.
# ============================================================================

# pkgver: strip the git-describe 'v' prefix and map '-' to '.' (makepkg forbids '-').
string(REGEX REPLACE "^v" "" QGC_ARCH_PKGVER "${QGC_APP_VERSION_STR}")
string(REPLACE "-" "." QGC_ARCH_PKGVER "${QGC_ARCH_PKGVER}")

# Single source of truth for the Arch runtime dep list (see PKGBUILD.in note):
# the remaining system libraries QGC links against once Qt is bundled by the
# CMake deploy step. makepkg runs --nodeps, so this is metadata only.
set(_qgc_arch_depends
    glibc gcc-libs gstreamer gst-plugins-base-libs openblas
    libpulse fontconfig freetype2 libglvnd libxkbcommon-x11
    libsm libusb vulkan-icd-loader hicolor-icon-theme)
set(QGC_ARCH_DEPENDS "")
foreach(_dep IN LISTS _qgc_arch_depends)
    string(APPEND QGC_ARCH_DEPENDS "'${_dep}' ")
endforeach()
string(STRIP "${QGC_ARCH_DEPENDS}" QGC_ARCH_DEPENDS)

set(_qgc_arch_dir "${CMAKE_BINARY_DIR}/arch-pkg")
configure_file(
    "${CMAKE_SOURCE_DIR}/deploy/linux/PKGBUILD.in"
    "${_qgc_arch_dir}/PKGBUILD"
    @ONLY
)

# PKGDEST drops the package in the build root (where find_artifact.py looks);
# BUILDDIR keeps makepkg scratch out of the source tree.
add_custom_target(
    qgc-package
    COMMAND "${CMAKE_COMMAND}" "-DQGC_NATIVE_PACKAGE_VERSION=${QGC_ARCH_PKGVER}" -P
            "${CMAKE_SOURCE_DIR}/cmake/install/ValidatePackageVersion.cmake"
    COMMAND "${CMAKE_COMMAND}" -E env "PKGDEST=${CMAKE_BINARY_DIR}" "BUILDDIR=${_qgc_arch_dir}" makepkg -f --noconfirm
            --nodeps --skipinteg
    WORKING_DIRECTORY "${_qgc_arch_dir}"
    VERBATIM
    COMMENT "QGC: building Arch package via makepkg"
)
