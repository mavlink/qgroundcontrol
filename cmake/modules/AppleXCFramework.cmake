include_guard(GLOBAL)
# cmake-lint: disable=R0915

# qgc_find_ios_xcframework_slice(XCFRAMEWORK <dir> PLATFORM <iphoneos|iphonesimulator> OUT_VAR <var>
#                                [ARCHITECTURES <arch>...])
# Sets <var> to the library directory (slice) of the xcframework for the target platform whose identifier names every
# ARCHITECTURES entry, the first in sorted order when several do; fails configure when none does. A slice holds a
# <name>.framework or, as in GStreamer's, a static library.
function(qgc_find_ios_xcframework_slice)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "" "XCFRAMEWORK;PLATFORM;OUT_VAR" "ARCHITECTURES")
    if(ARG_KEYWORDS_MISSING_VALUES OR ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "qgc_find_ios_xcframework_slice: malformed arguments "
                            "(missing='${ARG_KEYWORDS_MISSING_VALUES}', unknown='${ARG_UNPARSED_ARGUMENTS}')"
        )
    endif()
    foreach(required_arg IN ITEMS XCFRAMEWORK PLATFORM OUT_VAR)
        if(NOT ARG_${required_arg})
            message(FATAL_ERROR "qgc_find_ios_xcframework_slice: ${required_arg} is required")
        endif()
    endforeach()
    if(NOT ARG_OUT_VAR MATCHES "^[A-Za-z_][A-Za-z0-9_]*$")
        message(FATAL_ERROR "qgc_find_ios_xcframework_slice: invalid OUT_VAR '${ARG_OUT_VAR}'")
    endif()
    if(NOT IS_DIRECTORY "${ARG_XCFRAMEWORK}")
        message(FATAL_ERROR "qgc_find_ios_xcframework_slice: xcframework not found: ${ARG_XCFRAMEWORK}")
    endif()
    if(NOT ARG_PLATFORM MATCHES "^(iphoneos|iphonesimulator)$")
        message(FATAL_ERROR "qgc_find_ios_xcframework_slice: PLATFORM must be iphoneos or iphonesimulator, "
                            "got '${ARG_PLATFORM}'"
        )
    endif()

    cmake_path(GET ARG_XCFRAMEWORK STEM framework_name)
    if(ARG_PLATFORM STREQUAL "iphonesimulator")
        file(
            GLOB candidate_paths
            LIST_DIRECTORIES true
            "${ARG_XCFRAMEWORK}/ios-*-simulator"
        )
    else()
        file(
            GLOB candidate_paths
            LIST_DIRECTORIES true
            "${ARG_XCFRAMEWORK}/ios-*"
        )
        list(FILTER candidate_paths EXCLUDE REGEX "/ios-[^/]*-(simulator|maccatalyst)$")
    endif()

    if(ARG_ARCHITECTURES)
        set(architecture_matches "")
        foreach(candidate IN LISTS candidate_paths)
            cmake_path(GET candidate FILENAME slice_id)
            set(architecture_ok TRUE)
            foreach(architecture IN LISTS ARG_ARCHITECTURES)
                if(NOT architecture MATCHES "^[A-Za-z0-9_]+$")
                    message(FATAL_ERROR "qgc_find_ios_xcframework_slice: invalid architecture '${architecture}'")
                endif()
                if(NOT slice_id MATCHES "(^|[-_])${architecture}([-_]|$)")
                    set(architecture_ok FALSE)
                    break()
                endif()
            endforeach()
            if(architecture_ok)
                list(APPEND architecture_matches "${candidate}")
            endif()
        endforeach()
        set(candidate_paths "${architecture_matches}")
    endif()

    list(SORT candidate_paths)
    list(LENGTH candidate_paths candidate_count)
    if(candidate_count EQUAL 0)
        message(FATAL_ERROR "QGC: ${framework_name}.xcframework has no ${ARG_PLATFORM} slice "
                            "for architectures '${ARG_ARCHITECTURES}'"
        )
    endif()
    list(GET candidate_paths 0 selected)
    set(${ARG_OUT_VAR}
        "${selected}"
        PARENT_SCOPE
    )
endfunction()
