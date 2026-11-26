include_guard(GLOBAL)

# qgc_check_libarchive_decoders(<libarchive target> [<library>...])
# Fails the configure when the libarchive that QGCCompression links lacks zlib or liblzma: it would then decode gzip
# and xz, which every build reads, with an external program, which QGCCompression never registers. Only a probe that was
# built and ran can tell. One that cannot be built or run here, as in a cross build, gives a warning instead. Nothing is
# cached, so that a configure after the library is replaced checks it again. <library>... are linked with the probe if
# they are targets, for a static libarchive whose target does not name the decoder libraries.
function(qgc_check_libarchive_decoders libarchive)
    set(_cannot_check "QGC: cannot check that the libarchive found reads gzip and xz itself")
    if(CMAKE_CROSSCOMPILING)
        message(WARNING "${_cannot_check}: this is a cross build")
        return()
    endif()

    set(_libraries ${libarchive})
    foreach(_library IN LISTS ARGN)
        if(TARGET ${_library})
            list(APPEND _libraries ${_library})
        endif()
    endforeach()
    try_run(
        _ran
        _built
        SOURCE_FROM_CONTENT
        libarchive_decoders.c
        "#include <archive.h>
int main(void) { return ((archive_zlib_version() != 0) && (archive_liblzma_version() != 0)) ? 0 : 1; }
"
        NO_CACHE
        LOG_DESCRIPTION
        "Checking that libarchive reads gzip and xz"
        LINK_LIBRARIES
        ${_libraries}
        COMPILE_OUTPUT_VARIABLE _build_output
    )
    if(NOT _built)
        message(WARNING "${_cannot_check}: the check could not be built:\n${_build_output}")
    elseif(_ran STREQUAL "1")
        message(FATAL_ERROR "QGC: the libarchive found was built without zlib or liblzma, so it cannot read gzip or xz")
    elseif(NOT _ran STREQUAL "0")
        message(WARNING "${_cannot_check}: the check could not be run (${_ran})")
    endif()
endfunction()
