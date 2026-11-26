# Script mode: cmake -DAPP=<ELF binary> [-DNM=<nm>] -P BundledDecoderExports.cmake
# Fails when the binary exports a symbol of a decompression library that QGCCompression links statically (zlib,
# liblzma, libarchive, Zstandard, LZ4, bzip2). An ELF executable exports what it defines of a symbol that a shared
# library it links also uses: Qt, libpng, freetype and the like would then call this copy of zlib for those functions
# and the system's for the others, on the same stream. The Android application, a shared library, exports all of them.
# src/Utilities/Compression/CMakeLists.txt hides them with --exclude-libs on Linux and Android and runs this check after
# the link, with the toolchain's nm (llvm-nm for Android).
include_guard(GLOBAL)

# Names of the exported decoder symbols in the output of `nm -D --defined-only`, in the order nm lists them
function(qgc_bundled_decoder_exports nm_output out_var)
    set(_found "")
    string(REPLACE "\n" ";" _lines "${nm_output}")
    foreach(_line IN LISTS _lines)
        # "<address> <type> <name>[@version]"
        if(NOT _line MATCHES "^[0-9a-fA-F]* *[A-Za-z] ([^ @]+)")
            continue()
        endif()
        set(_name "${CMAKE_MATCH_1}")
        set(_zlib "^(adler32|compress|crc32|deflate|get_crc_table|gz[a-z_]+|inflate|uncompress|zError|zlib)")
        if(_name MATCHES "${_zlib}" OR _name MATCHES "^(lzma_|archive_|ZSTD_|ZDICT_|LZ4|BZ2_)")
            list(APPEND _found "${_name}")
        endif()
    endforeach()
    set(${out_var}
        "${_found}"
        PARENT_SCOPE
    )
endfunction()

if(CMAKE_SCRIPT_MODE_FILE STREQUAL CMAKE_CURRENT_LIST_FILE)
    if(NOT NM)
        set(NM nm)
    endif()
    execute_process(
        COMMAND "${NM}" -D --defined-only "${APP}"
        OUTPUT_VARIABLE _nm_output
        RESULT_VARIABLE _nm_result
    )
    if(NOT _nm_result EQUAL 0)
        message(FATAL_ERROR "Decoder export check: '${NM} -D --defined-only ${APP}' failed (${_nm_result})")
    endif()

    qgc_bundled_decoder_exports("${_nm_output}" _exports)
    if(_exports)
        message(
            FATAL_ERROR
                "Decoder export check: ${APP} exports symbols of the decompression libraries it links statically "
                "(${_exports}). Libraries loaded with it would mix them with their own copy. Hide them with "
                "--exclude-libs (src/Utilities/Compression/CMakeLists.txt)."
        )
    endif()
endif()
