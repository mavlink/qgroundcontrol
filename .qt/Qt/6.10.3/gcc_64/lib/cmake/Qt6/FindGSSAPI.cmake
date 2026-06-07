# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

find_package(PkgConfig QUIET)
pkg_check_modules(PC_GSSAPI QUIET "krb5-gssapi")
if (NOT PC_GSSAPI_FOUND)
    pkg_check_modules(PC_GSSAPI QUIET "mit-krb5-gssapi")
endif()

find_path(GSSAPI_INCLUDE_DIRS
          NAMES gssapi/gssapi.h
          HINTS ${PC_GSSAPI_INCLUDEDIR}
)

# On macOS, vcpkg opts for finding frameworks LAST. This is generally fine;
# however, in the case of GSSAPI, `usr/lib/libgssapi_krb5.tbd` which is a
# symlink to `Kerberos.framework` misses a few symols, e.g.,
# `___gss_c_nt_hostbased_service_oid_desc`, and it causes build failure.
# So, we need to make sure that we find `GSS.framework`.
set(gssapi_library_names
  GSS    # framework
  gss    # solaris
  gssapi # FreeBSD
  gssapi_krb5
)
if(APPLE)
  list(REMOVE_ITEM gssapi_library_names "gssapi_krb5")
endif()

find_library(GSSAPI_LIBRARIES
             NAMES
             ${gssapi_library_names}
             HINTS ${PC_GSSAPI_LIBDIR}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GSSAPI DEFAULT_MSG GSSAPI_LIBRARIES GSSAPI_INCLUDE_DIRS)

if(GSSAPI_FOUND AND NOT TARGET GSSAPI::GSSAPI)
    if(GSSAPI_LIBRARIES MATCHES "/([^/]+)\\.framework$")
        add_library(GSSAPI::GSSAPI INTERFACE IMPORTED)
        set_target_properties(GSSAPI::GSSAPI PROPERTIES
                              INTERFACE_LINK_LIBRARIES "${GSSAPI_LIBRARIES}")
    else()
      add_library(GSSAPI::GSSAPI UNKNOWN IMPORTED)
      set_target_properties(GSSAPI::GSSAPI PROPERTIES
                            IMPORTED_LOCATION "${GSSAPI_LIBRARIES}")
    endif()

    set_target_properties(GSSAPI::GSSAPI PROPERTIES
                          INTERFACE_INCLUDE_DIRECTORIES "${GSSAPI_INCLUDE_DIRS}")
endif()

mark_as_advanced(GSSAPI_INCLUDE_DIRS GSSAPI_LIBRARIES)

include(FeatureSummary)
set_package_properties(GSSAPI PROPERTIES
  DESCRIPTION "Generic Security Services Application Program Interface")
