# ============================================================================
# FFmpeg Video Receiver (experimental, QGC_ENABLE_FFMPEG_VIDEO, Linux only so far)
# Network streams received directly with the FFmpeg libraries Qt ships, for VideoSettings::videoBackend's FFmpeg
# choice. Links Qt's FFmpeg plugin implementation (Qt6::FFmpegMediaPluginImplPrivate), which is Qt-internal, and the
# FFmpeg libraries of the Qt kit. The kit has no FFmpeg headers: they are generated at configure time from the source
# of the FFmpeg release the kit's libraries are, into the build directory.
# ============================================================================

if(NOT QGC_ENABLE_FFMPEG_VIDEO)
    return()
endif()

if(NOT LINUX OR ANDROID)
    message(FATAL_ERROR "QGC: QGC_ENABLE_FFMPEG_VIDEO is only built for Linux so far")
endif()

# The FFmpeg release Qt 6.12.0's kit ships.
set(QGC_FFMPEG_TAG
    "n9.0.1"
    CACHE STRING "FFmpeg release whose headers the FFmpeg video receiver compiles against"
)
set(QGC_FFMPEG_SOURCE_SHA256
    "195d54bebe1a27f84d77f4b989d193466f305b355da92292766a69f16880b18a"
    CACHE STRING "SHA-256 of the source archive of QGC_FFMPEG_TAG"
)

set(_ffmpeg_qt_libdir "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_LIBS}")
set(_ffmpeg_prefix "${CMAKE_BINARY_DIR}/ffmpeg-${QGC_FFMPEG_TAG}")
set(_ffmpeg_libraries avformat avcodec avutil swresample swscale)

# --- Headers: from the release's source, through its configure and "make install-headers", which compile nothing.
if(NOT EXISTS "${_ffmpeg_prefix}/include/libavcodec/version_major.h")
    CPMAddPackage(
        NAME ffmpeg_source
        URL "https://github.com/FFmpeg/FFmpeg/archive/refs/tags/${QGC_FFMPEG_TAG}.tar.gz"
        URL_HASH "SHA256=${QGC_FFMPEG_SOURCE_SHA256}"
        DOWNLOAD_ONLY YES
    )
    qgc_require_cpm_added(ffmpeg_source)

    find_program(QGC_FFMPEG_MAKE NAMES make gmake REQUIRED)
    find_program(QGC_FFMPEG_SH NAMES sh bash REQUIRED)
    set(_ffmpeg_header_build "${CMAKE_BINARY_DIR}/ffmpeg-${QGC_FFMPEG_TAG}-headers-build")
    file(REMOVE_RECURSE "${_ffmpeg_header_build}")
    file(MAKE_DIRECTORY "${_ffmpeg_header_build}")
    message(STATUS "QGC: Generating the FFmpeg ${QGC_FFMPEG_TAG} headers")
    execute_process(
        COMMAND
            "${QGC_FFMPEG_SH}" "${ffmpeg_source_SOURCE_DIR}/configure" "--prefix=${_ffmpeg_prefix}"
            "--cc=${CMAKE_C_COMPILER}" --disable-programs --disable-doc --disable-avdevice --disable-avfilter
            --disable-autodetect --disable-x86asm --enable-shared --disable-static
        WORKING_DIRECTORY "${_ffmpeg_header_build}"
        RESULT_VARIABLE _ffmpeg_result
        OUTPUT_FILE "${_ffmpeg_header_build}/configure.log"
        ERROR_FILE "${_ffmpeg_header_build}/configure.log"
    )
    if(_ffmpeg_result EQUAL 0)
        execute_process(
            COMMAND "${QGC_FFMPEG_MAKE}" install-headers
            WORKING_DIRECTORY "${_ffmpeg_header_build}"
            RESULT_VARIABLE _ffmpeg_result
            OUTPUT_FILE "${_ffmpeg_header_build}/install-headers.log"
            ERROR_FILE "${_ffmpeg_header_build}/install-headers.log"
        )
    endif()
    if(NOT _ffmpeg_result EQUAL 0 OR NOT EXISTS "${_ffmpeg_prefix}/include/libavcodec/version_major.h")
        message(FATAL_ERROR "QGC: Generating the FFmpeg headers failed; see the logs in ${_ffmpeg_header_build}")
    endif()
    file(REMOVE_RECURSE "${_ffmpeg_header_build}")
endif()

# --- The headers must be those of the kit's libraries: their major versions name the libraries' ABI.
foreach(_lib avcodec avformat avutil)
    string(TOUPPER "${_lib}" _upper)
    set(_version_header "${_ffmpeg_prefix}/include/lib${_lib}/version_major.h")
    if(NOT EXISTS "${_version_header}")
        set(_version_header "${_ffmpeg_prefix}/include/lib${_lib}/version.h")
    endif()
    file(STRINGS "${_version_header}" _define REGEX "#define LIB${_upper}_VERSION_MAJOR +[0-9]+")
    string(REGEX MATCH "[0-9]+$" _header_major "${_define}")
    if(NOT EXISTS "${_ffmpeg_qt_libdir}/lib${_lib}.so")
        message(
            FATAL_ERROR "QGC: QGC_ENABLE_FFMPEG_VIDEO needs Qt's FFmpeg libraries; "
                        "${_ffmpeg_qt_libdir}/lib${_lib}.so is missing"
        )
    endif()
    get_filename_component(_library "${_ffmpeg_qt_libdir}/lib${_lib}.so" REALPATH)
    get_filename_component(_library_name "${_library}" NAME)
    string(REGEX MATCH "\\.so\\.([0-9]+)" _ "${_library_name}")
    if(NOT _header_major OR NOT "${_header_major}" STREQUAL "${CMAKE_MATCH_1}")
        message(
            FATAL_ERROR
                "QGC: The FFmpeg ${QGC_FFMPEG_TAG} headers are lib${_lib} ${_header_major}, but Qt's library is "
                "${_library_name}. Set QGC_FFMPEG_TAG and QGC_FFMPEG_SOURCE_SHA256 to the FFmpeg release of this Qt "
                "kit, and delete ${_ffmpeg_prefix}."
        )
    endif()
endforeach()

# --- Qt's FindFFmpeg (FFMPEG_DIR) wants the headers and libraries under one prefix, with a pkg-config file each.
file(MAKE_DIRECTORY "${_ffmpeg_prefix}/lib/pkgconfig")
foreach(_lib IN LISTS _ffmpeg_libraries)
    file(CREATE_LINK "${_ffmpeg_qt_libdir}/lib${_lib}.so" "${_ffmpeg_prefix}/lib/lib${_lib}.so" SYMBOLIC)
    file(
        WRITE "${_ffmpeg_prefix}/lib/pkgconfig/lib${_lib}.pc"
        "prefix=${_ffmpeg_prefix}\nlibdir=\${prefix}/lib\nincludedir=\${prefix}/include\n\n"
        "Name: lib${_lib}\nDescription: Qt's lib${_lib}\nVersion: ${QGC_FFMPEG_TAG}\n"
        "Libs: -L\${libdir} -l${_lib}\nCflags: -I\${includedir}\n"
    )
endforeach()
set(FFMPEG_DIR "${_ffmpeg_prefix}")
find_package(Qt6 ${QGC_QT_MINIMUM_VERSION} REQUIRED COMPONENTS FFmpegMediaPluginImplPrivate)
# The receiver's tests read its recordings back with libavformat.
set_target_properties(
    FFmpeg::avformat
    FFmpeg::avcodec
    FFmpeg::avutil
    PROPERTIES
        IMPORTED_GLOBAL TRUE
)

target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE
        FFmpegPointers.cc
        FFmpegPointers.h
        FFmpegQtInternals.cc
        FFmpegQtInternals.h
        FFmpegStreamRecorder.cc
        FFmpegStreamRecorder.h
        FFmpegStreamSession.cc
        FFmpegStreamSession.h
        FFmpegVideoReceiver.cc
        FFmpegVideoReceiver.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QGC_FFMPEG_VIDEO)
target_link_libraries(
    ${CMAKE_PROJECT_NAME}
    PRIVATE
        Qt6::FFmpegMediaPluginImplPrivate
        Qt6::MultimediaPrivate
        FFmpeg::avformat
        FFmpeg::avcodec
        FFmpeg::avutil
)
