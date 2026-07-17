include_guard(GLOBAL)

# Select the framework directory for an iOS xcframework and target platform.
# XCFramework library identifiers encode both platform and architectures.
function(qgc_find_ios_xcframework_slice)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "" "XCFRAMEWORK;PLATFORM;OUT_VAR" "ARCHITECTURES")
    if(ARG_KEYWORDS_MISSING_VALUES OR ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
            "qgc_find_ios_xcframework_slice: malformed arguments "
            "(missing='${ARG_KEYWORDS_MISSING_VALUES}', unknown='${ARG_UNPARSED_ARGUMENTS}')")
    endif()
    foreach(_required IN ITEMS XCFRAMEWORK PLATFORM OUT_VAR)
        if(NOT ARG_${_required})
            message(FATAL_ERROR "qgc_find_ios_xcframework_slice: ${_required} is required")
        endif()
    endforeach()
    if(NOT ARG_OUT_VAR MATCHES "^[A-Za-z_][A-Za-z0-9_]*$")
        message(FATAL_ERROR
            "qgc_find_ios_xcframework_slice: invalid OUT_VAR '${ARG_OUT_VAR}'")
    endif()
    if(NOT IS_DIRECTORY "${ARG_XCFRAMEWORK}")
        message(FATAL_ERROR
            "qgc_find_ios_xcframework_slice: xcframework not found: ${ARG_XCFRAMEWORK}")
    endif()
    if(NOT ARG_PLATFORM MATCHES "^(iphoneos|iphonesimulator)$")
        message(FATAL_ERROR
            "qgc_find_ios_xcframework_slice: PLATFORM must be iphoneos or iphonesimulator, "
            "got '${ARG_PLATFORM}'")
    endif()

    cmake_path(GET ARG_XCFRAMEWORK STEM _framework_name)
    if(ARG_PLATFORM STREQUAL "iphonesimulator")
        file(GLOB _candidates LIST_DIRECTORIES true
            "${ARG_XCFRAMEWORK}/ios-*-simulator/${_framework_name}.framework")
    else()
        file(GLOB _candidates LIST_DIRECTORIES true
            "${ARG_XCFRAMEWORK}/ios-*/${_framework_name}.framework")
        list(FILTER _candidates EXCLUDE REGEX "/ios-[^/]*-(simulator|maccatalyst)/")
    endif()

    if(ARG_ARCHITECTURES)
        set(_architecture_matches "")
        foreach(_candidate IN LISTS _candidates)
            cmake_path(GET _candidate PARENT_PATH _slice_dir)
            cmake_path(GET _slice_dir FILENAME _slice_id)
            set(_matches TRUE)
            foreach(_architecture IN LISTS ARG_ARCHITECTURES)
                if(NOT _architecture MATCHES "^[A-Za-z0-9_]+$")
                    message(FATAL_ERROR
                        "qgc_find_ios_xcframework_slice: invalid architecture '${_architecture}'")
                endif()
                if(NOT _slice_id MATCHES "(^|[-_])${_architecture}([-_]|$)")
                    set(_matches FALSE)
                    break()
                endif()
            endforeach()
            if(_matches)
                list(APPEND _architecture_matches "${_candidate}")
            endif()
        endforeach()
        set(_candidates "${_architecture_matches}")
    endif()

    list(SORT _candidates)
    list(LENGTH _candidates _candidate_count)
    if(_candidate_count EQUAL 0)
        message(FATAL_ERROR
            "QGC: ${_framework_name}.xcframework has no ${ARG_PLATFORM} slice "
            "for architectures '${ARG_ARCHITECTURES}'")
    endif()
    list(GET _candidates 0 _selected)
    set(${ARG_OUT_VAR} "${_selected}" PARENT_SCOPE)
endfunction()
