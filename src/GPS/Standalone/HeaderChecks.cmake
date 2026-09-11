include_guard(GLOBAL)

get_property(api_targets GLOBAL PROPERTY QGC_GPS_PUBLIC_API_TARGETS)
list(REMOVE_DUPLICATES api_targets)
foreach(api_target IN LISTS api_targets)
    get_target_property(headers ${api_target} QGC_PUBLIC_HEADER_NAMES)
    set(_sources)
    foreach(header IN LISTS headers)
        string(MAKE_C_IDENTIFIER "${header}" source_name)
        set(_source "${CMAKE_CURRENT_BINARY_DIR}/headers/${api_target}/${source_name}.cc")
        file(
            GENERATE
            OUTPUT "${_source}"
            CONTENT "#include <${header}>\n"
        )
        list(APPEND _sources "${_source}")
    endforeach()
    add_library(${api_target}Headers OBJECT ${_sources})
    set_target_properties(
        ${api_target}Headers
        PROPERTIES AUTOMOC OFF
                   AUTORCC OFF
                   AUTOUIC OFF
    )
    target_link_libraries(${api_target}Headers PRIVATE ${api_target})
    set(_main "${CMAKE_CURRENT_BINARY_DIR}/headers/${api_target}/_main.cc")
    file(
        GENERATE
        OUTPUT "${_main}"
        CONTENT "int main() { return 0; }\n"
    )
    add_executable(${api_target}Consumer "${_main}" $<TARGET_OBJECTS:${api_target}Headers>)
    get_target_property(library_type ${api_target} TYPE)
    if(library_type STREQUAL "STATIC_LIBRARY")
        # Resolve every object, including methods not referenced by an include-only translation unit.
        target_link_libraries(${api_target}Consumer PRIVATE "$<LINK_LIBRARY:WHOLE_ARCHIVE,${api_target}>")
    else()
        target_link_libraries(${api_target}Consumer PRIVATE ${api_target})
    endif()
    add_test(NAME ${api_target}Consumer COMMAND ${api_target}Consumer)
    set_tests_properties(${api_target}Consumer PROPERTIES LABELS "Unit;GPS;Library;PublicAPI" TIMEOUT 10)
endforeach()
