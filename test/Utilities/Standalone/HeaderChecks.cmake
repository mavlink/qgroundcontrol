# Compile each public header independently, then link a consumer using only its owning target.
function(qgc_check_utility_consumer target)
    get_target_property(headers ${target} HEADER_SET)
    set(sources)
    foreach(header IN LISTS headers)
        get_filename_component(header_name "${header}" NAME)
        set(source "${CMAKE_CURRENT_BINARY_DIR}/headers/${target}/${header_name}.cc")
        file(
            GENERATE
            OUTPUT "${source}"
            CONTENT "#include <${header_name}>\n"
        )
        list(APPEND sources "${source}")
    endforeach()
    add_library(${target}Headers OBJECT ${sources})
    set_target_properties(
        ${target}Headers
        PROPERTIES AUTOMOC OFF
                   AUTOUIC OFF
                   AUTORCC OFF
    )
    target_link_libraries(${target}Headers PRIVATE ${target})

    add_executable(${target}Consumer "${CMAKE_CURRENT_SOURCE_DIR}/Consumers/${target}.cc")
    set_target_properties(
        ${target}Consumer
        PROPERTIES AUTOMOC OFF
                   AUTOUIC OFF
                   AUTORCC OFF
    )
    target_link_libraries(${target}Consumer PRIVATE ${target})
    add_test(NAME ${target}Consumer COMMAND ${target}Consumer)
    set_tests_properties(${target}Consumer PROPERTIES LABELS "Unit;Utilities" TIMEOUT 30)
endfunction()
