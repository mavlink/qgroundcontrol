include_guard(GLOBAL)

# Add the archives and compressed files that the Compression suites read from :/unittest/ to the resources of target:
# the application, or a portable test executable. The resource is named after the target, because Qt names the file
# it generates after the resource alone, and several targets of one directory call this
function(qgc_add_compression_test_resources target)
    set(_files manifest.json.gz manifest.json.xz manifest.json.zip tree.zip encrypted.zip)
    # The archives and streams of the optional formats, which the tests read only when the build reads them
    if(QGC_ENABLE_7ZIP)
        list(APPEND _files manifest.json.7z tree.7z encrypted.7z)
    endif()
    if(QGC_ENABLE_BZIP2)
        list(APPEND _files manifest.json.bz2)
    endif()
    if(QGC_ENABLE_LZ4)
        list(APPEND _files manifest.json.lz4)
    endif()
    if(QGC_ENABLE_ZSTD)
        list(APPEND _files manifest.json.zst)
    endif()
    list(TRANSFORM _files PREPEND "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/")

    qt_add_resources(
        ${target} "${target}_CompressionTest_res"
        PREFIX "/unittest"
        BASE "${CMAKE_CURRENT_FUNCTION_LIST_DIR}"
        FILES
            ${_files}
    )
endfunction()
