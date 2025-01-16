include(CMakePrintHelpers)

# message(STATUS "Creating Mac Bundle")

# set(TARGET_NAME QGroundControl)
set(BUNDLE_PATH ${CMAKE_BINARY_DIR}/staging/${TARGET_NAME}.app)
set(SYSTEM_FRAMEWORK_PATH /Library/Frameworks)
set(BUNDLE_FRAMEWORK_PATH ${BUNDLE_PATH}/Contents/Frameworks)

# execute_process(
#     COMMAND ${CMAKE_SOURCE_DIR}/deploy/macos/prepare_gstreamer_framework.sh ${CMAKE_BINARY_DIR}/gstwork/ staging/${TARGET_NAME}.app ${TARGET_NAME}
#     RESULT_VARIABLE GSTREAMER_FRAMEWORK_RESULT
#     OUTPUT_VARIABLE GSTREAMER_FRAMEWORK_OUTPUT
#     ERROR_VARIABLE GSTREAMER_FRAMEWORK_ERROR
# )

# cmake_print_variables(GSTREAMER_FRAMEWORK_RESULT GSTREAMER_FRAMEWORK_OUTPUT GSTREAMER_FRAMEWORK_ERROR)

# message(STATUS "Copy GStreamer framework into bundle")
# file(
#     INSTALL "${SYSTEM_FRAMEWORK_PATH}/GStreamer.framework"
#     DESTINATION "${BUNDLE_FRAMEWORK_PATH}"
#     PATTERN "*.la" EXCLUDE
#     PATTERN "*.a" EXCLUDE
#     PATTERN "*/include" EXCLUDE
#     PATTERN "*/Headers" EXCLUDE
# )
# file(REMOVE_RECURSE ${BUNDLE_FRAMEWORK_PATH}/GStreamer.framework/Versions/1.0/bin)
# file(REMOVE_RECURSE ${BUNDLE_FRAMEWORK_PATH}/GStreamer.framework/Versions/1.0/etc)
# file(REMOVE_RECURSE ${BUNDLE_FRAMEWORK_PATH}/GStreamer.framework/Versions/1.0/share)
# file(REMOVE_RECURSE ${BUNDLE_FRAMEWORK_PATH}/GStreamer.framework/Versions/1.0/Headers)
# file(REMOVE_RECURSE ${BUNDLE_FRAMEWORK_PATH}/GStreamer.framework/Versions/1.0/include)
# file(REMOVE_RECURSE ${BUNDLE_FRAMEWORK_PATH}/GStreamer.framework/Versions/1.0/Commands)
# file(REMOVE_RECURSE ${BUNDLE_FRAMEWORK_PATH}/GStreamer.framework/Versions/1.0/lib/pkgconfig)
# file(REMOVE_RECURSE ${BUNDLE_FRAMEWORK_PATH}/GStreamer.framework/Versions/1.0/lib/glib-2.0)
# file(REMOVE_RECURSE ${BUNDLE_FRAMEWORK_PATH}/GStreamer.framework/Versions/1.0/lib/graphene-1.0)
# file(REMOVE_RECURSE ${BUNDLE_FRAMEWORK_PATH}/GStreamer.framework/Versions/1.0/lib/gst-validate-launcher)
# file(GLOB REMOVE_LIB_FILES ${BUNDLE_FRAMEWORK_PATH}/GStreamer.framework/Versions/1.0/lib/*.a)
# file(REMOVE ${REMOVE_LIB_FILES})
# file(GLOB REMOVE_LIB_FILES ${BUNDLE_FRAMEWORK_PATH}/GStreamer.framework/Versions/1.0/lib/*.la)
# file(REMOVE ${REMOVE_LIB_FILES})
# file(GLOB REMOVE_LIB_FILES ${BUNDLE_FRAMEWORK_PATH}/GStreamer.framework/Versions/1.0/lib/gstreamer-1.0/*.a)
# file(REMOVE ${REMOVE_LIB_FILES})
# file(GLOB REMOVE_LIB_FILES ${BUNDLE_FRAMEWORK_PATH}/GStreamer.framework/Versions/1.0/lib/gstreamer-1.0/*.la)
# file(REMOVE ${REMOVE_LIB_FILES})
# file(REMOVE_RECURSE ${BUNDLE_FRAMEWORK_PATH}/GStreamer.framework/Versions/1.0/lib/gstreamer-1.0/include)
# file(REMOVE_RECURSE ${BUNDLE_FRAMEWORK_PATH}/GStreamer.framework/Versions/1.0/lib/gstreamer-1.0/pkgconfig)

# execute_process(COMMAND otool -L ${SYSTEM_FRAMEWORK_PATH}/GStreamer.framework/GStreamer)

# Fix up library paths to point into bundle
# execute_process(
#     COMMAND ln -sf ${BUNDLE_FRAMEWORK_PATH} ${BUNDLE_FRAMEWORK_PATH}/GStreamer.framework/Versions/1.0/libexec
# )
# execute_process(
#     COMMAND install_name_tool -id @executable_path/../Frameworks/GStreamer.framework/Versions/1.0/lib/GStreamer ${BUNDLE_PATH}/Contents/Frameworks/GStreamer.framework/Versions/1.0/GStreamer
#     RESULT_VARIABLE GSTREAMER_FRAMEWORK_RESULT
#     OUTPUT_VARIABLE GSTREAMER_FRAMEWORK_OUTPUT
#     ERROR_VARIABLE GSTREAMER_FRAMEWORK_ERROR
# )
# execute_process(
#     COMMAND install_name_tool -change ${SYSTEM_FRAMEWORK_PATH}/GStreamer.framework/Versions/1.0/lib/GStreamer @executable_path/../Frameworks/GStreamer.framework/Versions/1.0/lib/GStreamer ${BUNDLE_PATH}/Contents/MacOS/${TARGET_NAME}
#     RESULT_VARIABLE GSTREAMER_FRAMEWORK_RESULT
#     OUTPUT_VARIABLE GSTREAMER_FRAMEWORK_OUTPUT
#     ERROR_VARIABLE GSTREAMER_FRAMEWORK_ERROR
# )

find_package(Python3 REQUIRED)
# set(OSXRELOCATOR ${CMAKE_SOURCE_DIR}/deploy/macos/osxrelocator.py)

execute_process(
    COMMAND ${Python3_EXECUTABLE} ${OSXRELOCATOR} ${BUNDLE_PATH}/Contents/Frameworks/GStreamer.framework/Versions/Current/lib /Library/Frameworks/GStreamer.framework/ @executable_path/../Frameworks/GStreamer.framework/ -r
    RESULT_VARIABLE PYTHON_RESULT
    OUTPUT_VARIABLE PYTHON_OUTPUT
    ERROR_VARIABLE PYTHON_ERROR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
cmake_print_variables(PYTHON_RESULT PYTHON_OUTPUT PYTHON_ERROR)

execute_process(
    COMMAND ${Python3_EXECUTABLE} ${OSXRELOCATOR} ${BUNDLE_PATH}/Contents/Frameworks/GStreamer.framework/Versions/Current/libexec /Library/Frameworks/GStreamer.framework/ @executable_path/../../../../../GStreamer.framework/ -r
    RESULT_VARIABLE PYTHON_RESULT
    OUTPUT_VARIABLE PYTHON_OUTPUT
    ERROR_VARIABLE PYTHON_ERROR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
cmake_print_variables(PYTHON_RESULT PYTHON_OUTPUT PYTHON_ERROR)

execute_process(
    COMMAND ${Python3_EXECUTABLE} ${OSXRELOCATOR} ${BUNDLE_PATH}/Contents/Frameworks/GStreamer.framework/Versions/Current/bin /Library/Frameworks/GStreamer.framework/ @executable_path/../../../../GStreamer.framework/ -r
    RESULT_VARIABLE PYTHON_RESULT
    OUTPUT_VARIABLE PYTHON_OUTPUT
    ERROR_VARIABLE PYTHON_ERROR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
cmake_print_variables(PYTHON_RESULT PYTHON_OUTPUT PYTHON_ERROR)

execute_process(
    COMMAND ${Python3_EXECUTABLE} ${OSXRELOCATOR} ${BUNDLE_PATH}/Contents/MacOS /Library/Frameworks/GStreamer.framework/ @executable_path/../Frameworks/GStreamer.framework/ -r
    RESULT_VARIABLE PYTHON_RESULT
    OUTPUT_VARIABLE PYTHON_OUTPUT
    ERROR_VARIABLE PYTHON_ERROR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
cmake_print_variables(PYTHON_RESULT PYTHON_OUTPUT PYTHON_ERROR)

execute_process(COMMAND otool -L ${BUNDLE_PATH}/Contents/MacOS/${TARGET_NAME})

# include(BundleUtilities)

# fixup_bundle("${BUNDLE_PATH}" "" "${SYSTEM_FRAMEWORK_PATH}/GStreamer.framework/Versions/1.0/lib/GStreamer")
# verify_app("${BUNDLE_PATH}")
# verify_bundle_prerequisites("${BUNDLE_PATH}" VERIFY_BUNDLE_PREREQS_RESULT VERIFY_BUNDLE_PREREQS_INFO)
# cmake_print_variables(VERIFY_BUNDLE_PREREQS_RESULT VERIFY_BUNDLE_PREREQS_INFO)
# verify_bundle_symlinks("${BUNDLE_PATH}" VERIFY_BUNDLE_SYMLINKS_RESULT VERIFY_BUNDLE_SYMLINKS_INFO)
# cmake_print_variables(VERIFY_BUNDLE_SYMLINKS_RESULT VERIFY_BUNDLE_SYMLINKS_INFO)

message(STATUS "Creating Mac DMG")
file(REMOVE_RECURSE ${CMAKE_BINARY_DIR}/package)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/package)
file(COPY ${BUNDLE_PATH} DESTINATION ${CMAKE_BINARY_DIR}/package)
execute_process(COMMAND create-dmg --volname "${TARGET_NAME}" --filesystem "APFS" "${TARGET_NAME}.dmg" "${CMAKE_BINARY_DIR}/package/")
