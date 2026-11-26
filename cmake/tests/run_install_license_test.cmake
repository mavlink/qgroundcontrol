# qgc_install_license() (Helpers.cmake) installs a library's licence where the packages of each platform carry it: the
# fixture checks the APK assets and the iOS bundle when it configures, and this script the desktop and macOS installs
if(NOT QGC_MODULE_DIR
   OR NOT FIXTURE_SOURCE_DIR
   OR NOT FIXTURE_BINARY_DIR
)
    message(FATAL_ERROR "QGC_MODULE_DIR, FIXTURE_SOURCE_DIR, and FIXTURE_BINARY_DIR are required")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" --fresh -S "${FIXTURE_SOURCE_DIR}" -B "${FIXTURE_BINARY_DIR}"
            "-DQGC_MODULE_DIR=${QGC_MODULE_DIR}" -DTEST_CASE=helpers-install-license
    RESULT_VARIABLE _configure_result
    OUTPUT_VARIABLE _configure_output
    ERROR_VARIABLE _configure_error
)
if(NOT _configure_result EQUAL 0)
    message(FATAL_ERROR "Licence fixture configure failed:\n${_configure_output}${_configure_error}")
endif()

# The .deb, .rpm and Windows packages hold only the Runtime component
set(_stage "${FIXTURE_BINARY_DIR}/stage")
file(REMOVE_RECURSE "${_stage}")
execute_process(
    COMMAND "${CMAKE_COMMAND}" --install "${FIXTURE_BINARY_DIR}" --prefix "${_stage}" --component Runtime
    RESULT_VARIABLE _install_result
    OUTPUT_VARIABLE _install_output
    ERROR_VARIABLE _install_error
)
if(NOT _install_result EQUAL 0)
    message(FATAL_ERROR "Licence fixture install failed:\n${_install_output}${_install_error}")
endif()

foreach(
    _path IN ITEMS share/licenses/example/LICENSE
                   QGCCMakeModuleContracts.app/Contents/Resources/licenses/example-macos/LICENSE
)
    if(NOT EXISTS "${_stage}/${_path}")
        message(FATAL_ERROR "ASSERT [installed licence]: ${_path} is missing from the Runtime install")
    endif()
endforeach()
