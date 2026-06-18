# ============================================================================
# CreateArchPackage.cmake
# Arch Linux package via makepkg (CPack has no pacman generator). Defines the
# `qgc-package` target. Smoke artifact, not a published distributable, so
# makepkg failure is non-fatal — the AppImage is the real Arch deliverable.
# ============================================================================

# pkgver: strip the git-describe 'v' prefix and map '-' to '.' (makepkg forbids '-').
string(REGEX REPLACE "^v" "" QGC_ARCH_PKGVER "${QGC_APP_VERSION_STR}")
string(REPLACE "-" "." QGC_ARCH_PKGVER "${QGC_ARCH_PKGVER}")
if(QGC_ARCH_PKGVER STREQUAL "")
    set(QGC_ARCH_PKGVER "0.0.0")
endif()

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
# BUILDDIR keeps makepkg scratch out of the source tree. `|| echo` keeps a
# makepkg failure non-fatal so `--target qgc-package` still succeeds.
add_custom_target(qgc-package
    COMMAND ${CMAKE_COMMAND} -E env
        "PKGDEST=${CMAKE_BINARY_DIR}" "BUILDDIR=${_qgc_arch_dir}"
        bash -c "makepkg -f --noconfirm --nodeps --skipinteg || echo 'QGC: makepkg failed; Arch package skipped (AppImage still produced).' >&2"
    WORKING_DIRECTORY "${_qgc_arch_dir}"
    VERBATIM
    COMMENT "QGC: building Arch package via makepkg (smoke artifact)"
)
