# TODO: # go-appimage, updateinformation w/ GitHub Releases, signing

message(STATUS "QGC: Creating AppImage")

set(APPDIR_PATH "${CMAKE_BINARY_DIR}/AppDir")
set(APPIMAGE_PATH "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}-${CMAKE_SYSTEM_PROCESSOR}.AppImage")

#===========================================================================#
# Download Tools

function(download_tool VAR URL)
    cmake_path(GET URL FILENAME _name)
    set(_dest "${CMAKE_BINARY_DIR}/tools/${_name}")
    if(NOT EXISTS "${_dest}")
        file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/tools")
        message(STATUS "QGC: Downloading ${_name} to ${_dest}")
        file(DOWNLOAD "${URL}" "${_dest}" STATUS _status)
        list(GET _status 0 _result)
        if(NOT _result EQUAL 0)
            message(FATAL_ERROR "Failed to download ${URL} to ${_dest}: ${_status}")
        endif()
        file(CHMOD "${_dest}" FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    endif()
    set(${VAR}_PATH "${_dest}" PARENT_SCOPE)
endfunction()

download_tool(LINUXDEPLOY   https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-${CMAKE_SYSTEM_PROCESSOR}.AppImage)
download_tool(APPIMAGETOOL  https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-${CMAKE_SYSTEM_PROCESSOR}.AppImage)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
    download_tool(APPIMAGELINT  https://github.com/TheAssassin/appimagelint/releases/download/continuous/appimagelint-${CMAKE_SYSTEM_PROCESSOR}.AppImage)
endif()

#===========================================================================#
# Bundle the runtime

execute_process(
    COMMAND "${LINUXDEPLOY_PATH}"
            --appdir "${APPDIR_PATH}"
            --executable "${APPDIR_PATH}/usr/bin/${CMAKE_PROJECT_NAME}"
            --desktop-file "${APPDIR_PATH}/usr/share/applications/${QGC_PACKAGE_NAME}.desktop"
            --custom-apprun "${CMAKE_BINARY_DIR}/AppRun"
            --icon-file "${APPDIR_PATH}/usr/share/icons/hicolor/256x256/apps/${CMAKE_PROJECT_NAME}.png"
    COMMAND_ECHO STDOUT
    COMMAND_ERROR_IS_FATAL ANY
)

#===========================================================================#
# Bundle libstdc++ and libgcc_s explicitly to improve compatibility

# Try to locate libstdc++.so.6 and libgcc_s.so.1 from the active toolchain
set(_stdcxx "")
set(_libgcc "")
execute_process(
    COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=libstdc++.so.6
    OUTPUT_VARIABLE _stdcxx
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND ${CMAKE_C_COMPILER} -print-file-name=libgcc_s.so.1
    OUTPUT_VARIABLE _libgcc
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

file(MAKE_DIRECTORY "${APPDIR_PATH}/usr/lib")

if(EXISTS "${_stdcxx}" AND IS_ABSOLUTE "${_stdcxx}")
    message(STATUS "QGC: Bundling libstdc++ from ${_stdcxx}")
    file(COPY "${_stdcxx}" DESTINATION "${APPDIR_PATH}/usr/lib")
else()
    message(WARNING "QGC: Could not find libstdc++.so.6 to bundle (looked at '${_stdcxx}')")
endif()

if(EXISTS "${_libgcc}" AND IS_ABSOLUTE "${_libgcc}")
    message(STATUS "QGC: Bundling libgcc_s from ${_libgcc}")
    file(COPY "${_libgcc}" DESTINATION "${APPDIR_PATH}/usr/lib")
else()
    message(WARNING "QGC: Could not find libgcc_s.so.1 to bundle (looked at '${_libgcc}')")
endif()

#===========================================================================#
# Build the final AppImage

set(ENV{ARCH} ${CMAKE_SYSTEM_PROCESSOR})
set(ENV{VERSION} ${CMAKE_PROJECT_VERSION})
execute_process(
    COMMAND "${APPIMAGETOOL_PATH}" "${APPDIR_PATH}" "${APPIMAGE_PATH}"
    COMMAND_ECHO STDOUT
    COMMAND_ERROR_IS_FATAL ANY
)

#===========================================================================#
# Lint

if(EXISTS "${APPIMAGELINT_PATH}")
    execute_process(
        COMMAND "${APPIMAGELINT_PATH}" "${APPIMAGE_PATH}"
        RESULT_VARIABLE LINT_RESULT
        COMMAND_ECHO STDOUT
    )
    if(NOT LINT_RESULT EQUAL 0)
        message(WARNING "QGC: appimagelint reported problems; see log above")
    endif()
endif()
