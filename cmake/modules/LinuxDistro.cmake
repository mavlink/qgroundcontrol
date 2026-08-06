# Detect host Linux distro from /etc/os-release for distro-appropriate defaults
# (native builds only; Docker passes packaging flags via -D). Sets QGC_LINUX_DISTRO[_LIKE/_FAMILY].

function(_qgc_detect_linux_distro)
    set(_distro "unknown")
    set(_like "")
    if(EXISTS "/etc/os-release")
        file(STRINGS "/etc/os-release" _osrel)
        foreach(_line IN LISTS _osrel)
            if(_line MATCHES "^ID=(.*)$")
                set(_distro "${CMAKE_MATCH_1}")
            elseif(_line MATCHES "^ID_LIKE=(.*)$")
                set(_like "${CMAKE_MATCH_1}")
            endif()
        endforeach()
        # file(STRINGS) keeps a trailing CR on CRLF os-release files; strip it
        # first so the anchored quote/family regexes still match.
        string(REGEX REPLACE "[\r\n]+$" "" _distro "${_distro}")
        string(REGEX REPLACE "[\r\n]+$" "" _like "${_like}")
        # os-release values may be quoted; strip surrounding quotes and lowercase.
        string(REGEX REPLACE "^\"(.*)\"$" "\\1" _distro "${_distro}")
        string(REGEX REPLACE "^\"(.*)\"$" "\\1" _like "${_like}")
        string(TOLOWER "${_distro}" _distro)
        string(TOLOWER "${_like}" _like)
    endif()

    set(_family "")
    if(_distro MATCHES "^(ubuntu|debian|linuxmint|pop|raspbian)$" OR _like MATCHES "debian")
        set(_family "debian")
    elseif(_distro MATCHES "^(fedora|rhel|centos|rocky|almalinux)$" OR _like MATCHES "rhel|fedora")
        set(_family "rhel")
    elseif(_distro MATCHES "^(arch|manjaro|endeavouros)$" OR _like MATCHES "arch")
        set(_family "arch")
    elseif(_distro MATCHES "^(opensuse.*|sles)$" OR _like MATCHES "suse")
        set(_family "suse")
    endif()

    set(QGC_LINUX_DISTRO "${_distro}" PARENT_SCOPE)
    set(QGC_LINUX_DISTRO_LIKE "${_like}" PARENT_SCOPE)
    set(QGC_LINUX_DISTRO_FAMILY "${_family}" PARENT_SCOPE)
endfunction()

_qgc_detect_linux_distro()
