# Acquiring a GStreamer SDK: qgc_gst_acquire_sdk downloads a platform's packages (Packages.cmake) into
# CPM_SOURCE_CACHE and extracts them once into the SDK tree, and the per-platform acquirers in acquire/<platform>.cmake,
# included at the end, name the packages, the cache directory and the extractor. CI keys its cache of the extracted
# trees on this file, acquire/, Packages.cmake and the gstreamer section of .github/build-config.json
# (.github/actions/cache/action.yml), so keep everything that decides an extracted tree in those files and nothing
# else.

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
        qgc_gst_get_fallback_checksum(${_platform} ${version} _pin)
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
        qgc_gst_get_package_url(${_platform} ${version} _url)
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

foreach(_qgc_gst_acquire_platform IN ITEMS Windows MacOS Android IOS)
    include("${CMAKE_CURRENT_LIST_DIR}/acquire/${_qgc_gst_acquire_platform}.cmake")
endforeach()
