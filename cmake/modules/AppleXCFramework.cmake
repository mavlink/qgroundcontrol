include_guard(GLOBAL)
# cmake-lint: disable=R0915

# Select the framework directory for an iOS xcframework and target platform.
# XCFramework library identifiers encode both platform and architectures.
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
            "${ARG_XCFRAMEWORK}/ios-*-simulator/${framework_name}.framework"
        )
    else()
        file(
            GLOB candidate_paths
            LIST_DIRECTORIES true
            "${ARG_XCFRAMEWORK}/ios-*/${framework_name}.framework"
        )
        list(FILTER candidate_paths EXCLUDE REGEX "/ios-[^/]*-(simulator|maccatalyst)/")
    endif()

    if(ARG_ARCHITECTURES)
        set(architecture_matches "")
        foreach(candidate IN LISTS candidate_paths)
            cmake_path(GET candidate PARENT_PATH slice_dir)
            cmake_path(GET slice_dir FILENAME slice_id)
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
