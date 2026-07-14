# Keep the bundled application runtime private while exposing freedesktop
# metadata and a launcher in their standard system locations.

set(_qgc_app_prefix "opt/QGroundControl")

if(DEFINED QGC_NATIVE_PACKAGE_ROOT AND NOT QGC_NATIVE_PACKAGE_ROOT STREQUAL "")
    set(_qgc_package_roots "${QGC_NATIVE_PACKAGE_ROOT}")
elseif(DEFINED CPACK_TEMPORARY_DIRECTORY AND IS_DIRECTORY "${CPACK_TEMPORARY_DIRECTORY}")
    # Locate generator-specific component roots from the installed executable.
    file(
        GLOB_RECURSE _qgc_staged_entries
        LIST_DIRECTORIES false
        "${CPACK_TEMPORARY_DIRECTORY}/*"
    )
    set(_qgc_package_roots "")
    foreach(qgc_entry IN LISTS _qgc_staged_entries)
        if(qgc_entry MATCHES "/${_qgc_app_prefix}/bin/QGroundControl$")
            string(REGEX REPLACE "/${_qgc_app_prefix}/bin/QGroundControl$" "" _qgc_root "${qgc_entry}")
            list(APPEND _qgc_package_roots "${_qgc_root}")
        endif()
    endforeach()
    list(REMOVE_DUPLICATES _qgc_package_roots)
else()
    message(FATAL_ERROR "QGC: native package root is unavailable")
endif()

if(NOT _qgc_package_roots)
    message(FATAL_ERROR "QGC: no staged /${_qgc_app_prefix}/bin/QGroundControl executable found")
endif()

foreach(qgc_root IN LISTS _qgc_package_roots)
    set(_qgc_private_root "${qgc_root}/${_qgc_app_prefix}")
    if(NOT EXISTS "${_qgc_private_root}/bin/QGroundControl")
        message(FATAL_ERROR "QGC: incomplete private runtime at ${_qgc_private_root}")
    endif()
    file(GLOB _qgc_qt_core "${_qgc_private_root}/lib/libQt6Core.so*")
    if(NOT _qgc_qt_core)
        message(FATAL_ERROR "QGC: bundled Qt Core is missing under ${_qgc_private_root}/lib")
    endif()

    file(MAKE_DIRECTORY "${qgc_root}/usr/share")
    foreach(qgc_data_dir IN ITEMS applications icons metainfo)
        set(_qgc_source "${_qgc_private_root}/share/${qgc_data_dir}")
        set(_qgc_destination "${qgc_root}/usr/share/${qgc_data_dir}")
        if(NOT EXISTS "${_qgc_source}")
            message(FATAL_ERROR "QGC: required native package metadata is missing: ${_qgc_source}")
        endif()
        if(EXISTS "${_qgc_destination}")
            message(FATAL_ERROR "QGC: native package destination already exists: ${_qgc_destination}")
        endif()
        file(RENAME "${_qgc_source}" "${_qgc_destination}")
    endforeach()

    foreach(qgc_required_path IN ITEMS
            "applications/org.mavlink.qgroundcontrol.desktop"
            "icons/hicolor/256x256/apps/QGroundControl.png"
            "icons/hicolor/scalable/apps/QGroundControl.svg"
            "metainfo/org.mavlink.qgroundcontrol.appdata.xml"
    )
        if(NOT EXISTS "${qgc_root}/usr/share/${qgc_required_path}")
            message(FATAL_ERROR
                "QGC: required native package payload is missing: /usr/share/${qgc_required_path}"
            )
        endif()
    endforeach()

    file(
        GLOB _qgc_remaining_share_entries
        LIST_DIRECTORIES true
        "${_qgc_private_root}/share/*"
    )
    if(NOT _qgc_remaining_share_entries)
        file(REMOVE_RECURSE "${_qgc_private_root}/share")
    endif()

    file(MAKE_DIRECTORY "${qgc_root}/usr/bin")
    set(_qgc_launcher "${qgc_root}/usr/bin/QGroundControl")
    file(REMOVE "${_qgc_launcher}")
    file(CREATE_LINK "../../${_qgc_app_prefix}/bin/QGroundControl" "${_qgc_launcher}" SYMBOLIC RESULT _qgc_link_result)
    if(NOT _qgc_link_result STREQUAL "0")
        message(FATAL_ERROR "QGC: failed to create native package launcher: ${_qgc_link_result}")
    endif()

    message(STATUS "QGC: finalized private native runtime under /${_qgc_app_prefix}")
endforeach()
