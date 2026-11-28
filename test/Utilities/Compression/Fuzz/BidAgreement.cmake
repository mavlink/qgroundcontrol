include_guard(GLOBAL)

# Give target what BidAgreement.cc needs: the private headers of QGCCompression and libarchive's own. The target adds
# BidAgreement.cc itself, or has it already
function(qgc_add_bid_agreement target)
    target_include_directories(
        ${target}
        PRIVATE
            ${CMAKE_CURRENT_FUNCTION_LIST_DIR}
            ${PROJECT_SOURCE_DIR}/src/Utilities/Compression/private
    )
    target_link_libraries(
        ${target}
        PRIVATE
            LibArchive::LibArchive
            QGCCompression
    )
    if(TARGET archive_static)
        target_compile_definitions(${target} PRIVATE LIBARCHIVE_STATIC)
    endif()
endfunction()
