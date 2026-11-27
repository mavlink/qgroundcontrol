# ============================================================================
# FFmpeg Video Receiver (experimental, QGC_ENABLE_FFMPEG_VIDEO, desktop Linux and Android so far)
# Network streams received directly with the FFmpeg libraries Qt ships, for VideoSettings::videoBackend's FFmpeg
# choice. Links Qt's FFmpeg plugin implementation (Qt6::FFmpegMediaPluginImplPrivate), which is Qt-internal, and the
# FFmpeg libraries of the Qt kit. The kit has no FFmpeg headers: they are generated at configure time from the source
# of the FFmpeg release the kit's libraries are, into the build directory.
# ============================================================================

if(NOT QGC_ENABLE_FFMPEG_VIDEO)
    return()
endif()

# The option is ON by default, so a build that can't have the receiver goes without it: another platform, a Qt that
# uses the system's FFmpeg rather than its own, a host without the tools that generate the headers (a Windows host
# counts as one), or a cross build other than Android's, as FFmpeg's configure runs what it compiles unless it is told
# the target.
if(NOT LINUX AND NOT ANDROID)
    return()
endif()
set(_ffmpeg_qt_libdir "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_LIBS}")
set(_ffmpeg_libraries avformat avcodec avutil swresample swscale)
set(_ffmpeg_missing "")
foreach(_lib IN LISTS _ffmpeg_libraries)
    if(NOT EXISTS "${_ffmpeg_qt_libdir}/lib${_lib}.so")
        set(_ffmpeg_missing "the Qt kit has no lib${_lib}.so")
    endif()
endforeach()
if(NOT EXISTS "${_ffmpeg_qt_libdir}/cmake/Qt6FFmpegMediaPluginImplPrivate")
    set(_ffmpeg_missing "the Qt kit has no Qt6FFmpegMediaPluginImplPrivate package")
endif()
find_program(QGC_FFMPEG_MAKE NAMES make gmake)
find_program(QGC_FFMPEG_SH NAMES sh bash)
if(NOT QGC_FFMPEG_MAKE OR NOT QGC_FFMPEG_SH)
    set(_ffmpeg_missing "make and sh, which generate the FFmpeg headers, were not found")
endif()
if(CMAKE_CROSSCOMPILING AND NOT ANDROID)
    set(_ffmpeg_missing "its headers are not generated in a cross build")
endif()
if(CMAKE_HOST_WIN32)
    # Untried: an Android build on Windows, where FFmpeg's configure would run in whatever sh was found.
    set(_ffmpeg_missing "its headers are not generated on a Windows host")
endif()
if(_ffmpeg_missing)
    message(STATUS "QGC: FFmpeg video backend not built: ${_ffmpeg_missing}")
    return()
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

set(_ffmpeg_prefix "${CMAKE_BINARY_DIR}/ffmpeg-${QGC_FFMPEG_TAG}")

# The headers come from the release's source, through its configure and "make install-headers", which compile nothing.
if(NOT EXISTS "${_ffmpeg_prefix}/include/libavcodec/version_major.h")
    CPMAddPackage(
        NAME ffmpeg_source
        URL "https://github.com/FFmpeg/FFmpeg/archive/refs/tags/${QGC_FFMPEG_TAG}.tar.gz"
        URL_HASH "SHA256=${QGC_FFMPEG_SOURCE_SHA256}"
        DOWNLOAD_ONLY YES
    )
    qgc_require_cpm_added(ffmpeg_source)

    set(_ffmpeg_header_build "${CMAKE_BINARY_DIR}/ffmpeg-${QGC_FFMPEG_TAG}-headers-build")
    file(REMOVE_RECURSE "${_ffmpeg_header_build}")
    file(MAKE_DIRECTORY "${_ffmpeg_header_build}")
    if(ANDROID)
        # A cross build, with the NDK's compiler for the ABI: configure then runs nothing of what it compiles.
        set(_ffmpeg_target
            --enable-cross-compile
            --target-os=android
            "--arch=${CMAKE_SYSTEM_PROCESSOR}"
            "--cc=${CMAKE_C_COMPILER} --target=${CMAKE_C_COMPILER_TARGET}"
            "--ar=${CMAKE_AR}"
            "--nm=${CMAKE_NM}"
            "--ranlib=${CMAKE_RANLIB}"
            "--strip=${CMAKE_STRIP}"
            --disable-asm
        )
    else()
        set(_ffmpeg_target "--cc=${CMAKE_C_COMPILER}" --disable-x86asm)
    endif()
    message(STATUS "QGC: Generating the FFmpeg ${QGC_FFMPEG_TAG} headers")
    execute_process(
        COMMAND
            "${QGC_FFMPEG_SH}" "${ffmpeg_source_SOURCE_DIR}/configure" "--prefix=${_ffmpeg_prefix}" ${_ffmpeg_target}
            --disable-programs --disable-doc --disable-avdevice --disable-avfilter --disable-autodetect --enable-shared
            --disable-static
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

# The headers must be those of the kit's libraries: their major versions name the libraries' ABI.
set(_ffmpeg_header_majors "")
foreach(_lib avcodec avformat avutil)
    string(TOUPPER "${_lib}" _upper)
    set(_version_header "${_ffmpeg_prefix}/include/lib${_lib}/version_major.h")
    if(NOT EXISTS "${_version_header}")
        set(_version_header "${_ffmpeg_prefix}/include/lib${_lib}/version.h")
    endif()
    file(STRINGS "${_version_header}" _define REGEX "#define LIB${_upper}_VERSION_MAJOR +[0-9]+")
    string(REGEX MATCH "[0-9]+$" _header_major "${_define}")
    list(APPEND _ffmpeg_header_majors "QGC_FFMPEG_LIB${_upper}_MAJOR=${_header_major}")
    get_filename_component(_library "${_ffmpeg_qt_libdir}/lib${_lib}.so" REALPATH)
    get_filename_component(_library_name "${_library}" NAME)
    set(_library_major "")
    if(_library_name MATCHES "\\.so\\.([0-9]+)")
        set(_library_major "${CMAKE_MATCH_1}")
    else()
        # Android's libraries have no version in their names. libavcodec and libavformat hold theirs as a string
        # ("Lavc63.1.101"); libavutil holds none, and goes unchecked there.
        file(
            STRINGS "${_library}" _ident
            REGEX "^Lav[cf][0-9]+\\.[0-9]+\\.[0-9]+$"
            LIMIT_COUNT 1
        )
        if(_ident MATCHES "^Lav[cf]([0-9]+)\\.")
            set(_library_major "${CMAKE_MATCH_1}")
        elseif(_lib STREQUAL "avutil")
            continue()
        endif()
    endif()
    if(NOT _header_major OR NOT "${_header_major}" STREQUAL "${_library_major}")
        message(
            FATAL_ERROR
                "QGC: The FFmpeg ${QGC_FFMPEG_TAG} headers are lib${_lib} ${_header_major}, but Qt's library is "
                "${_library_name}. Set QGC_FFMPEG_TAG and QGC_FFMPEG_SOURCE_SHA256 to the FFmpeg release of this Qt "
                "kit and delete ${_ffmpeg_prefix}, or configure with -DQGC_ENABLE_FFMPEG_VIDEO=OFF."
        )
    endif()
endforeach()

# Qt's FindFFmpeg (FFMPEG_DIR) wants the headers and libraries under one prefix, with a pkg-config file each.
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
# Global, as the targets Qt found at the top level are: the application target, which links these, is not of this
# directory, and its AUTOMOC dependencies are resolved in its own.
set(QT_PROMOTE_TO_GLOBAL_TARGETS TRUE)
find_package(Qt6 ${QGC_QT_MINIMUM_VERSION} REQUIRED COMPONENTS FFmpegMediaPluginImplPrivate)
# Linked in place of Qt's library is a copy without what must not come along with it:
# - Its debug information. Qt builds the library with it and an archive keeps it, which put 43 MB into a Release
#   executable that has none of its own.
# - On Android, its JNI_OnLoad(). Qt's library defines an exported one in an object file that comes along with
#   everything the receiver uses of it, and QGroundControl has its own (src/Android/AndroidInit.cc): the link would
#   fail on the duplicate. In the copy Qt's is local to that object file. Nothing calls it then, and nothing needs to:
#   FFmpegStreamSession gives FFmpeg the Java VM, and the native methods it registers are the plugin's, which
#   registers its own.
# Without an objcopy, off Android, Qt's library is linked as it is.
if(CMAKE_OBJCOPY OR ANDROID)
    get_target_property(_ffmpeg_impl Qt6::FFmpegMediaPluginImplPrivate IMPORTED_LOCATION)
    cmake_path(GET _ffmpeg_impl FILENAME _ffmpeg_impl_name)
    set(_ffmpeg_impl_copy "${CMAKE_CURRENT_BINARY_DIR}/${_ffmpeg_impl_name}")
    set(_ffmpeg_impl_copy_options --strip-debug)
    if(ANDROID)
        list(APPEND _ffmpeg_impl_copy_options --localize-symbol=JNI_OnLoad)
    endif()
    add_custom_command(
        OUTPUT "${_ffmpeg_impl_copy}"
        COMMAND "${CMAKE_OBJCOPY}" ${_ffmpeg_impl_copy_options} "${_ffmpeg_impl}" "${_ffmpeg_impl_copy}"
        DEPENDS "${_ffmpeg_impl}"
        COMMENT "Copying ${_ffmpeg_impl_name} for the FFmpeg receiver"
        VERBATIM
    )
    add_custom_target(
        QGCFFmpegMediaPluginImpl
        DEPENDS "${_ffmpeg_impl_copy}"
        COMMENT "Qt's FFmpeg plugin library as the FFmpeg receiver links it"
    )
    # Qt gives the one library as the location for each configuration and for none.
    set_target_properties(
        Qt6::FFmpegMediaPluginImplPrivate
        PROPERTIES
            IMPORTED_LOCATION "${_ffmpeg_impl_copy}"
    )
    get_target_property(_ffmpeg_impl_configs Qt6::FFmpegMediaPluginImplPrivate IMPORTED_CONFIGURATIONS)
    foreach(_config IN LISTS _ffmpeg_impl_configs)
        set_target_properties(
            Qt6::FFmpegMediaPluginImplPrivate
            PROPERTIES
                IMPORTED_LOCATION_${_config} "${_ffmpeg_impl_copy}"
        )
    endforeach()
    add_dependencies(Qt6::FFmpegMediaPluginImplPrivate QGCFFmpegMediaPluginImpl)
endif()
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
# Ahead of every other directory: GStreamer's Android SDK has the headers of its own FFmpeg, another release with
# other struct layouts, and the system may have one's too. FFmpegPointers.cc checks which ones the compiler found.
target_include_directories(${CMAKE_PROJECT_NAME} BEFORE PRIVATE "${_ffmpeg_prefix}/include")
target_compile_definitions(
    ${CMAKE_PROJECT_NAME}
    PRIVATE
        QGC_FFMPEG_VIDEO
        ${_ffmpeg_header_majors}
)
# For the tests and the configure summary, which can't tell from the option that the receiver is in the build.
set_property(GLOBAL PROPERTY QGC_FFMPEG_VIDEO_BUILT TRUE)
target_link_libraries(
    ${CMAKE_PROJECT_NAME}
    PRIVATE
        Qt6::FFmpegMediaPluginImplPrivate
        Qt6::MultimediaPrivate
        FFmpeg::avformat
        FFmpeg::avcodec
        FFmpeg::avutil
)
