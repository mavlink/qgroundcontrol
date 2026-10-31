# Acquiring a GStreamer SDK: qgc_gst_acquire_sdk downloads a platform's packages (Packages.cmake) into
# CPM_SOURCE_CACHE and extracts them once into the SDK tree, and the per-platform acquirers below name the packages,
# the cache directory and the extractor. CI keys its cache of the extracted trees on this file, Packages.cmake and the
# gstreamer section of .github/build-config.json (.github/actions/cache/action.yml), so keep everything that decides
# an extracted tree in those two files and nothing else.

include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/Packages.cmake")

# _qgc_gst_extract_once(<dir> <stamp> <extract_function> <archives>)
# Publishes <dir> exactly once per <stamp>: a matching <dir>/.complete means it is already extracted.
# Otherwise <extract_function>(<staging> <archives> <name>), <name> being <dir>'s file name, must create
# and fill <staging>, which is then stamped and renamed to <dir>, so <dir> only ever exists complete. The
# caller must hold the cache lock (see qgc_gst_acquire_sdk).
function(_qgc_gst_extract_once dir stamp extract_function archives)
    set(_stamp_file "${dir}/.complete")
    if(EXISTS "${_stamp_file}")
        file(READ "${_stamp_file}" _existing_stamp)
        if(_existing_stamp STREQUAL stamp)
            return()
        endif()
        message(STATUS "GStreamer: ${dir} was extracted from another archive; extracting again")
    elseif(EXISTS "${dir}")
        message(STATUS "GStreamer: ${dir} has no completion stamp; extracting again")
    endif()

    set(_staging "${dir}.partial")
    file(REMOVE_RECURSE "${dir}" "${_staging}")
    cmake_path(GET dir FILENAME _name)
    cmake_language(CALL ${extract_function} "${_staging}" "${archives}" "${_name}")
    if(NOT IS_DIRECTORY "${_staging}")
        message(FATAL_ERROR "GStreamer: ${extract_function} did not create ${_staging}")
    endif()
    file(WRITE "${_staging}/.complete" "${stamp}")

    # Freshly written files can briefly hold the directory open on Windows (e.g. antivirus scans).
    foreach(_attempt IN ITEMS 1 2 3 4 5)
        file(RENAME "${_staging}" "${dir}" RESULT _rename_result)
        if(_rename_result EQUAL 0)
            return()
        endif()
        execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 2)
    endforeach()
    message(FATAL_ERROR "GStreamer: could not move ${_staging} to ${dir}: ${_rename_result}")
endfunction()

# _qgc_gst_pinned_stamp(<platform> <version> <stamp_out> <archive>...)
# Sets <stamp_out> to the completion stamp (see _qgc_gst_extract_once) of the <archive>s, the primary one for <platform>
# and a second one for <platform>_devel, when every one is pinned in build-config.json, else to "".
function(_qgc_gst_pinned_stamp platform version stamp_out)
    set(_platforms "${platform}" "${platform}_devel")
    set(_stamp "")
    foreach(_archive IN LISTS ARGN)
        list(POP_FRONT _platforms _platform)
        gstreamer_get_fallback_checksum(${_platform} ${version} _pin)
        if(NOT _pin)
            set(_stamp "")
            break()
        endif()
        string(APPEND _stamp "${_archive}=${_pin}\n")
    endforeach()
    set(${stamp_out}
        "${_stamp}"
        PARENT_SCOPE
    )
endfunction()

# _qgc_gst_download_archives(<dir> <platform> <version> <paths_out> <stamp_out> <archive>...)
# Downloads the <archive>s, the primary one for <platform> and a second one for <platform>_devel, into <dir>; sets
# <paths_out> to their paths and <stamp_out> to their completion stamp, each archive's line naming the checksum it was
# verified against, else its own SHA256.
function(_qgc_gst_download_archives dir platform version paths_out stamp_out)
    set(_platforms "${platform}" "${platform}_devel")
    set(_paths "")
    set(_stamp "")
    foreach(_archive IN LISTS ARGN)
        list(POP_FRONT _platforms _platform)
        _qgc_gst_download_archive(${_platform} ${version} "${_archive}" "${dir}" _hash)
        if(NOT _hash)
            file(SHA256 "${dir}/${_archive}" _sha256)
            set(_hash "SHA256=${_sha256}")
        endif()
        list(APPEND _paths "${dir}/${_archive}")
        string(APPEND _stamp "${_archive}=${_hash}\n")
    endforeach()
    set(${paths_out}
        "${_paths}"
        PARENT_SCOPE
    )
    set(${stamp_out}
        "${_stamp}"
        PARENT_SCOPE
    )
endfunction()

# _qgc_gst_record_packages(<platform> <version> <stamp>)
# Records the upstream packages a completion stamp (see _qgc_gst_extract_once) names, the primary one for <platform>
# and a second one for <platform>_devel, as "<url>|<sha256>" entries in the INTERNAL cache entry
# QGC_GST_RESOLVED_PACKAGES, which the SDK's notice (Notice.cmake) and SBOM entry (generate_cpm_sbom.py) name.
function(_qgc_gst_record_packages platform version stamp)
    # CI tooling reads the name from CMakeCache.txt, so it keeps the public QGC_ prefix like SdkState.cmake's entries.
    # cmake-lint: disable=C0103
    set(_platforms "${platform}" "${platform}_devel")
    set(_packages "")
    string(REGEX MATCHALL "=SHA256=[0-9a-fA-F]+" _hashes "${stamp}")
    foreach(_hash IN LISTS _hashes)
        list(POP_FRONT _platforms _platform)
        gstreamer_get_package_url(${_platform} ${version} _url)
        string(REPLACE "=SHA256=" "" _sha256 "${_hash}")
        list(APPEND _packages "${_url}|${_sha256}")
    endforeach()
    set(QGC_GST_RESOLVED_PACKAGES
        "${_packages}"
        CACHE INTERNAL "Upstream packages (<url>|<sha256>) of the auto-downloaded GStreamer SDK"
    )
endfunction()

# qgc_gst_acquire_sdk(PLATFORM <platform> VERSION <version> CACHE_SUBDIR <subdir>
#                     ARCHIVES <primary> [<devel>] CONTENT <name>
#                     EXTRACT_FUNCTION <command> CONTENT_DIR_OUT <var>)
# Downloads the SDK archive(s) for <platform> (the second one as <platform>_devel) into
# <cache>/<subdir> and extracts them once, with <command>(<staging> <archives> <name>), into
# <cache>/<subdir>/<name>, returned in CONTENT_DIR_OUT. <subdir> must name the version. When every archive is pinned,
# a tree already extracted from them is used without downloading them.
# <cache> is CPM_SOURCE_CACHE, which the top-level CMakeLists.txt always sets, so build directories and CI jobs
# share it. Concurrent configures are serialized on <cache>/<subdir>/.lock, and the extracted
# tree is stamped with the archive hashes; see _qgc_gst_extract_once. <command> sees
# _qgc_gst_archives_pinned TRUE when every archive was verified against its checksum pinned in build-config.json.
# Records the packages in QGC_GST_RESOLVED_PACKAGES (_qgc_gst_record_packages).
function(qgc_gst_acquire_sdk)
    cmake_parse_arguments(
        PARSE_ARGV 0 ARG "" "PLATFORM;VERSION;CACHE_SUBDIR;CONTENT;EXTRACT_FUNCTION;CONTENT_DIR_OUT" "ARCHIVES"
    )
    _qgc_gst_check_args(
        qgc_gst_acquire_sdk
        PLATFORM
        VERSION
        CACHE_SUBDIR
        ARCHIVES
        CONTENT
        EXTRACT_FUNCTION
        CONTENT_DIR_OUT
    )
    list(LENGTH ARG_ARCHIVES _archive_count)
    if(_archive_count GREATER 2)
        message(FATAL_ERROR "qgc_gst_acquire_sdk: ARCHIVES takes a primary and an optional devel archive")
    endif()
    if(NOT COMMAND ${ARG_EXTRACT_FUNCTION})
        message(FATAL_ERROR "qgc_gst_acquire_sdk: EXTRACT_FUNCTION '${ARG_EXTRACT_FUNCTION}' is not a command")
    endif()

    set(_cache_dir "${CPM_SOURCE_CACHE}/${ARG_CACHE_SUBDIR}")
    file(MAKE_DIRECTORY "${_cache_dir}")
    set(_lock "${_cache_dir}/.lock")
    file(
        LOCK "${_lock}"
        GUARD FUNCTION
        TIMEOUT 0
        RESULT_VARIABLE _lock_result
    )
    if(NOT _lock_result EQUAL 0)
        message(STATUS "GStreamer: waiting for another configure to release ${_lock}")
        file(
            LOCK "${_lock}"
            GUARD FUNCTION
            TIMEOUT 1800
            RESULT_VARIABLE _lock_result
        )
        if(NOT _lock_result EQUAL 0)
            message(FATAL_ERROR "GStreamer: could not lock ${_lock}: ${_lock_result}")
        endif()
    endif()

    # A tree stamped with the pinned checksums is used without hashing, or even having, its archives: CI caches keep
    # only the tree.
    _qgc_gst_pinned_stamp(${ARG_PLATFORM} ${ARG_VERSION} _pinned_stamp ${ARG_ARCHIVES})
    set(_content_dir "${_cache_dir}/${ARG_CONTENT}")
    if(_pinned_stamp AND EXISTS "${_content_dir}/.complete")
        file(READ "${_content_dir}/.complete" _existing_stamp)
        if(_existing_stamp STREQUAL _pinned_stamp)
            _qgc_gst_record_packages(${ARG_PLATFORM} ${ARG_VERSION} "${_pinned_stamp}")
            set(${ARG_CONTENT_DIR_OUT}
                "${_content_dir}"
                PARENT_SCOPE
            )
            return()
        endif()
    endif()
    _qgc_gst_download_archives("${_cache_dir}" ${ARG_PLATFORM} ${ARG_VERSION} _archive_paths _stamp ${ARG_ARCHIVES})
    set(_qgc_gst_archives_pinned FALSE)
    if(_pinned_stamp)
        set(_qgc_gst_archives_pinned TRUE)
    endif()
    _qgc_gst_extract_once("${_content_dir}" "${_stamp}" ${ARG_EXTRACT_FUNCTION} "${_archive_paths}")
    _qgc_gst_record_packages(${ARG_PLATFORM} ${ARG_VERSION} "${_stamp}")
    set(${ARG_CONTENT_DIR_OUT}
        "${_content_dir}"
        PARENT_SCOPE
    )
endfunction()

# Sets <out_var> TRUE when <root_dir> holds a usable MSVC SDK (pkg-config, headers, plugins and gstreamer-1.0.pc).
function(_qgc_gst_windows_sdk_complete root_dir out_var)
    set(_required_paths "bin/pkg-config.exe" "include/gstreamer-1.0" "lib/gstreamer-1.0"
                        "lib/pkgconfig/gstreamer-1.0.pc"
    )

    set(_is_complete TRUE)
    foreach(_path IN LISTS _required_paths)
        if(NOT EXISTS "${root_dir}/${_path}")
            set(_is_complete FALSE)
            break()
        endif()
    endforeach()

    set(${out_var}
        "${_is_complete}"
        PARENT_SCOPE
    )
endfunction()

# _qgc_gst_windows_extract(<staging> <installer>)
# qgc_gst_acquire_sdk extractor: runs the SDK installer silently into <staging>, which becomes the SDK root, and checks
# the result.
function(_qgc_gst_windows_extract staging installer)
    cmake_path(NATIVE_PATH installer _installer_native)
    cmake_path(NATIVE_PATH staging _staging_native)
    cmake_path(GET staging PARENT_PATH _cache_dir)
    set(_installer_log "${_cache_dir}/installer.log")
    cmake_path(NATIVE_PATH _installer_log _installer_log_native)

    # The .exe is auto-run with /VERYSILENT below. Only its in-tree pin authenticates it: GStreamer_REQUIRE_CHECKSUM=OFF
    # also accepts upstream's same-origin .sha256sum sidecar, or no checksum at all.
    if(NOT _qgc_gst_archives_pinned)
        message(
            FATAL_ERROR
                "GStreamer: refusing to silently run the downloaded Windows installer "
                "'${_installer_native}' because its checksum is not pinned in .github/build-config.json "
                "(gstreamer.checksums), so its integrity was not verified. Pin it or install the SDK "
                "manually and point GStreamer_ROOT_DIR at it."
        )
    endif()

    # /portable=1 (cerbero's Inno script, 1.28+) skips the uninstaller, the GSTREAMER_1_0_ROOT_* environment
    # variable, the InstallDir registry key, the VC++ redistributable and the Visual Studio templates. Without
    # it they would record <staging>, which is renamed once the install completes. /CURRENTUSER avoids
    # elevation; nothing is written outside <staging>.
    message(STATUS "Installing GStreamer SDK (silent)...")
    execute_process(
        COMMAND "${_installer_native}" /VERYSILENT /SUPPRESSMSGBOXES /SP- /NORESTART /CURRENTUSER /portable=1
                "/LOG=${_installer_log_native}" "/DIR=${_staging_native}"
        RESULT_VARIABLE _installer_rc
        ERROR_VARIABLE _installer_err
        TIMEOUT 600
    )
    if(NOT _installer_rc EQUAL 0)
        message(
            FATAL_ERROR "GStreamer installer failed (exit code: ${_installer_rc}).\n" "stderr: ${_installer_err}\n"
                        "See installer log: ${_installer_log}"
        )
    endif()

    _qgc_gst_windows_sdk_complete("${staging}" _complete)
    if(NOT _complete)
        message(
            FATAL_ERROR "GStreamer SDK installed into ${staging} but required files are missing.\n"
                        "See installer log: ${_installer_log}"
        )
    endif()
endfunction()

# _qgc_gst_acquire_windows_sdk(<arch> <out_root>)
# Downloads and silently installs the official MSVC SDK for <arch> (x86_64 or arm64); sets <out_root> to its root.
function(_qgc_gst_acquire_windows_sdk arch out_root)
    if(arch STREQUAL "arm64")
        set(_platform windows_msvc_arm64)
    else()
        set(_platform windows_msvc_x64)
    endif()
    qgc_gst_acquire_sdk(
        PLATFORM ${_platform}
        VERSION ${QGC_GST_REQUESTED_VERSION}
        CACHE_SUBDIR "gstreamer-win-${arch}-${QGC_GST_REQUESTED_VERSION}"
        ARCHIVES "gstreamer-${arch}.exe"
        CONTENT install
        EXTRACT_FUNCTION _qgc_gst_windows_extract
        CONTENT_DIR_OUT _root
    )
    set(${out_root}
        "${_root}"
        PARENT_SCOPE
    )
endfunction()

# Expands <pkg> with pkgutil --expand-full into <expanded_dir> and fails configure unless that yields package payloads.
# Used by the macOS (runtime+devel) and iOS SDK extractors; qgc_gst_acquire_sdk clears their staging directory, which
# holds <expanded_dir>, before every attempt.
function(_qgc_gst_pkgutil_expand_and_validate pkg expanded_dir label)
    message(STATUS "Expanding GStreamer ${label} package...")
    execute_process(COMMAND pkgutil --expand-full "${pkg}" "${expanded_dir}" RESULT_VARIABLE _pkgutil_rc)
    if(NOT _pkgutil_rc EQUAL 0)
        message(FATAL_ERROR "pkgutil failed to expand GStreamer ${label} .pkg (exit code: ${_pkgutil_rc})")
    endif()
    file(GLOB _payloads "${expanded_dir}/*.pkg/Payload")
    if(NOT _payloads)
        message(FATAL_ERROR "pkgutil expanded GStreamer ${label} package but no payloads were found in ${expanded_dir}")
    endif()
endfunction()

# _qgc_gst_macos_extract(<staging> <runtime_pkg;devel_pkg>)
# qgc_gst_acquire_sdk extractor: expands both .pkg files into <staging> and merges their payloads
# into the flat SDK root <staging>/root.
function(_qgc_gst_macos_extract staging archives)
    list(GET archives 0 _runtime_pkg)
    list(GET archives 1 _devel_pkg)
    set(_root "${staging}/root")
    file(MAKE_DIRECTORY "${_root}")
    _qgc_gst_pkgutil_expand_and_validate("${_runtime_pkg}" "${staging}/expanded" "macOS runtime")
    _qgc_gst_pkgutil_expand_and_validate("${_devel_pkg}" "${staging}/expanded-devel" "macOS devel")

    foreach(_expanded_dir IN ITEMS "${staging}/expanded" "${staging}/expanded-devel")
        file(GLOB _sub_pkg_dirs "${_expanded_dir}/*.pkg")
        foreach(_pkg_dir IN LISTS _sub_pkg_dirs)
            if(EXISTS "${_pkg_dir}/Payload")
                file(GLOB _payload_entries "${_pkg_dir}/Payload/*")
                foreach(_entry IN LISTS _payload_entries)
                    cmake_path(GET _entry FILENAME _entry_name)
                    if(_entry_name STREQUAL "Headers")
                        continue()
                    endif()
                    cmake_path(IS_PREFIX _root "${_root}/${_entry_name}" NORMALIZE _is_safe)
                    if(NOT _is_safe)
                        message(FATAL_ERROR "GStreamer: Path traversal detected in extracted SDK entry: ${_entry_name}")
                    endif()
                    if(IS_SYMLINK "${_entry}")
                        get_filename_component(_link_target "${_entry}" REALPATH)
                        cmake_path(IS_PREFIX _expanded_dir "${_link_target}" NORMALIZE _link_safe)
                        if(NOT _link_safe)
                            message(
                                FATAL_ERROR "GStreamer: SDK entry '${_entry_name}' symlinks outside the payload: "
                                            "${_link_target}"
                            )
                        endif()
                    endif()
                    file(COPY "${_entry}" DESTINATION "${_root}")
                endforeach()
            endif()
        endforeach()
    endforeach()

    foreach(_required IN ITEMS lib/gstreamer-1.0 include/gstreamer-1.0 lib/pkgconfig/gstreamer-1.0.pc)
        if(NOT EXISTS "${_root}/${_required}")
            message(
                FATAL_ERROR
                    "Downloaded macOS GStreamer SDK is incomplete (${_required} not found).\n"
                    "Install manually from https://gstreamer.freedesktop.org/download/ or set GStreamer_ROOT_DIR."
            )
        endif()
    endforeach()
endfunction()

# _qgc_gst_acquire_macos_sdk(<out_root>)
# Downloads the official macOS runtime and devel packages and merges them into a flat SDK; sets <out_root> to its root.
function(_qgc_gst_acquire_macos_sdk out_root)
    qgc_gst_acquire_sdk(
        PLATFORM macos
        VERSION ${QGC_GST_REQUESTED_VERSION}
        CACHE_SUBDIR "gstreamer-mac-${QGC_GST_REQUESTED_VERSION}"
        ARCHIVES gstreamer.pkg gstreamer-devel.pkg
        CONTENT sdk
        EXTRACT_FUNCTION _qgc_gst_macos_extract
        CONTENT_DIR_OUT _sdk
    )
    set(${out_root}
        "${_sdk}/root"
        PARENT_SCOPE
    )
endfunction()

# _qgc_gst_android_extract(<staging> <archive> <abi_dir>)
# qgc_gst_acquire_sdk extractor: unpacks only the <abi_dir>/ tree of the universal SDK tarball into
# <staging>. Every ABI build (including Qt's multi-ABI sub-builds) acquires its own tree.
function(_qgc_gst_android_extract staging archive abi_dir)
    # cmake-lint: disable=E1126
    file(
        ARCHIVE_EXTRACT
        INPUT
        "${archive}"
        DESTINATION
        "${staging}"
        PATTERNS
        "${abi_dir}/*"
    )
    if(NOT EXISTS "${staging}/${abi_dir}/lib/pkgconfig/gstreamer-1.0.pc")
        message(FATAL_ERROR "GStreamer: ${archive} has no ${abi_dir}/ SDK tree")
    endif()
endfunction()

# _qgc_gst_acquire_android_sdk(<abi_dir> <out_root>)
# Downloads the universal Android SDK and extracts its <abi_dir> tree (_qgc_gst_android_abi_info); sets <out_root> to
# that tree.
function(_qgc_gst_acquire_android_sdk abi_dir out_root)
    qgc_gst_acquire_sdk(
        PLATFORM android
        VERSION ${QGC_GST_REQUESTED_VERSION}
        CACHE_SUBDIR "gstreamer-android-${QGC_GST_REQUESTED_VERSION}"
        ARCHIVES "gstreamer-android-${QGC_GST_REQUESTED_VERSION}.tar.xz"
        CONTENT ${abi_dir}
        EXTRACT_FUNCTION _qgc_gst_android_extract
        CONTENT_DIR_OUT _sdk
    )
    set(${out_root}
        "${_sdk}/${abi_dir}"
        PARENT_SCOPE
    )
endfunction()

# _qgc_gst_ios_extract(<staging> <pkg>)
# qgc_gst_acquire_sdk extractor: expands the iOS .pkg into <staging>/expanded.
function(_qgc_gst_ios_extract staging pkg)
    file(MAKE_DIRECTORY "${staging}")
    _qgc_gst_pkgutil_expand_and_validate("${pkg}" "${staging}/expanded" "iOS")
endfunction()

# _qgc_gst_acquire_ios_sdk(<out_dir>)
# Downloads and expands the official iOS package; sets <out_dir> to the directory holding its expanded contents.
function(_qgc_gst_acquire_ios_sdk out_dir)
    qgc_gst_acquire_sdk(
        PLATFORM ios
        VERSION ${QGC_GST_REQUESTED_VERSION}
        CACHE_SUBDIR "gstreamer-ios-${QGC_GST_REQUESTED_VERSION}"
        ARCHIVES gstreamer-ios.pkg
        CONTENT sdk
        EXTRACT_FUNCTION _qgc_gst_ios_extract
        CONTENT_DIR_OUT _sdk
    )
    set(${out_dir}
        "${_sdk}/expanded"
        PARENT_SCOPE
    )
endfunction()
