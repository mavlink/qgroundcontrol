# cerbero - a multi-platform build system for Open Source software
# Copyright (C) 2012 Andoni Morales Alastruey <ylatuya@gmail.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public
# License along with this library; if not, write to the
# Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.

################
#  pkg-config  #
################
# Host tools
# Make pkg-config relocatable
# set PKG_CONFIG_LIBDIR and override the prefix and libdir variables
ifeq ($(HOST_OS),windows)
    HOST_PKG_CONFIG := $(GSTREAMER_NDK_BUILD_PATH)/tools/windows/pkg-config
    # No space before the &&, or it will be added to PKG_CONFIG_LIBDIR
    PKG_CONFIG_ORIG := set PKG_CONFIG_LIBDIR=$(GSTREAMER_ROOT)/lib/pkgconfig&& $(HOST_PKG_CONFIG)
    GSTREAMER_ROOT := $(subst \,/,$(GSTREAMER_ROOT))
else
    HOST_PKG_CONFIG := pkg-config
    PKG_CONFIG_ORIG := PKG_CONFIG_LIBDIR=$(GSTREAMER_ROOT)/lib/pkgconfig $(HOST_PKG_CONFIG)
endif

PKG_CONFIG := $(PKG_CONFIG_ORIG) --define-variable=prefix=$(GSTREAMER_ROOT) --define-variable=libdir=$(GSTREAMER_ROOT)/lib

# -----------------------------------------------------------------------------
# Function : pkg-config-get-includes
# Arguments: 1: package or list of packages
# Returns  : list of includes
# Usage    : $(call pkg-config-get-includes,<package>)
# -----------------------------------------------------------------------------
pkg-config-get-includes = \
  $(shell $(PKG_CONFIG) --cflags-only-I $1)

# -----------------------------------------------------------------------------
# Function : pkg-config-get-libs
# Arguments: 1: package or list of packages
# Returns  : list of libraries to link this package
# Usage    : $(call pkg-config-get-libs,<package>)
# -----------------------------------------------------------------------------
pkg-config-get-libs = \
  $(shell $(PKG_CONFIG) --libs-only-l $1)

# -----------------------------------------------------------------------------
# Function : pkg-config-get-libs-no-deps
# Arguments: 1: package or list of packages
# Returns  : list of -lfoo libraries for this packages without the deps
# Usage    : $(call pkg-config-get-libs,<package>)
# -----------------------------------------------------------------------------
pkg-config-get-libs-no-deps = \
  $(eval __tmpvar.libs := ) \
  $(foreach package,$1,\
    $(eval __tmpvar.libs += $(shell $(HOST_SED) -n 's/^Libs: \(.*\)/\1/p' $(GSTREAMER_ROOT)/lib/pkgconfig/$(package).pc)))\
  $(filter -l%, $(__tmpvar.libs))

# -----------------------------------------------------------------------------
# Function : pkg-config-get-prefix
# Arguments: 1: package
# Returns  : a string with the prefix variable
# Usage    : $(call pkg-config-get-prefix,<package>)
# -----------------------------------------------------------------------------
pkg-config-get-prefix = \
  $(shell $(HOST_SED) -n 's/^prefix=\(.*\)/\1/p' $(GSTREAMER_ROOT)/lib/pkgconfig/$1.pc)

# -----------------------------------------------------------------------------
# Function : libtool-whole-archive
# Arguments: 1: link command
#            2: list of libraries for which we want to include the whole archive
# Returns  : the fixed link command
# Usage    : $(call libtool-link,<cmd>,<libs>)
# -----------------------------------------------------------------------------
WHOLE_ARCHIVE = -Wl,--whole-archive
NO_WHOLE_ARCHIVE = -Wl,--no-whole-archive
libtool-whole-archive = \
  $(eval __tmpvar.archives_paths := ) \
  $(foreach lib,$2, \
    $(eval __tmpvar.archives_paths += $(patsubst %.la,%.a,$(call libtool-find-lib,$(patsubst -l%,%,$(lib)))))) \
  $(eval __tmpvar.cmd := $1) \
  $(foreach ar,$(__tmpvar.archives_paths), \
    $(eval __tmpvar.cmd := $(patsubst %$(ar),$(WHOLE_ARCHIVE) %$(ar) $(NO_WHOLE_ARCHIVE),$(__tmpvar.cmd)))\
  )\
  $(call __libtool_log, "Link Command with whole archives:" $(__tmpvar.cmd))\
  $(__tmpvar.cmd)

# -----------------------------------------------------------------------------
# Function : libtool-link
# Arguments: 1: link command
# Returns  : a link command with all the dependencies resolved as done by libtool
# Usage    : $(call libtool-link,<lib>)
# -----------------------------------------------------------------------------
libtool-link = \
  $(call libtool-clear-vars)\
  $(eval __libtool.link.command := $1)\
  $(call __libtool_log, original link command = $(__libtool.link.command))\
  $(eval __libtool.link.Lpath := $(call libtool-get-search-paths,$1))\
  $(call __libtool_log, Library Search Paths = $(__libtool.link.Lpath))\
  $(eval __libtool.link.libs := $(call libtool-get-libs,$1))\
  $(call __libtool_log, Libraries = $(__libtool.link.libs))\
  $(foreach library,$(__libtool.link.libs),$(call libtool-parse-lib,$(library)))\
  $(call libtool-gen-link-command)


###############################################################################
#                                                                             #
#            This functions are private, don't use them directly              #
#                                                                             #
###############################################################################

# -----------------------------------------------------------------------------
# Function : libtool-parse-library
# Arguments: 1: library name
# Returns  : ""
# Usage    : $(call libtool-parse-library,<libname>)
# Note     : Tries to find a libtool library for this name in the libraries search
#            path and parses it as well as its dependencies
# -----------------------------------------------------------------------------
libtool-parse-lib = \
  $(eval __tmpvar := $(strip $(call libtool-find-lib,$(patsubst -l%,%,$1))))\
  $(if $(__tmpvar), \
    $(call libtool-parse-file,$(__tmpvar),$(call libtool-name-from-filepath,$(__tmpvar))),\
    $(eval __libtool.link.shared_libs += $1)\
    $(call __libtool_log, libtool file not found for "$1" and will be added to the shared libs)\
  )

# -----------------------------------------------------------------------------
# Function : libtool-parse-file
# Arguments: 1: libtool file
#            2: library name
# Returns  : ""
# Usage    : $(call libtool-parse-file,<file>,<libname>)
# Note     :
#            Parses a libtool library and its dependencies recursively
#
#            For each library it sets the following variables:
#            __libtool_libs.libname.LIBS              -> non-libtool libraries linked with -lfoo
#            __libtool_libs.libname.STATIC_LIB        -> link statically this library
#            __libtool_libs.libname.DYN_LIB           -> link dynamically this library
#            __libtool_libs.libname.LIBS_SEARCH_PATH  -> libraries search path
#
#            Processed libraries are stored in __libtool_libs.processed, and
#            the list of libraries ordered by dependencies are stored in
#            __libtool_lbs.ordered
# -----------------------------------------------------------------------------
libtool-parse-file = \
  $(call __libtool_log, parsing file $1)\
  $(if $(call libtool-lib-processed,$2),\
      $(call __libtool_log, library $2 already parsed),\
    $(eval __libtool_libs.$2.STATIC_LIB := $(patsubst %.la,%.a,$1))\
    $(eval __libtool_libs.$2.DYN_LIB := -l$2)\
    $(eval __tmpvar.$2.dep_libs := $(call libtool-get-dependency-libs,$1))\
    $(eval __tmpvar.$2.dep_libs := $(call libtool-replace-prefixes,$(__tmpvar.$2.dep_libs)))\
    $(eval __libtool_libs.$2.LIBS := $(call libtool-get-libs,$(__tmpvar.$2.dep_libs)))\
    $(eval __libtool_libs.$2.LIBS_SEARCH_PATH := $(call libtool-get-search-paths,$(__tmpvar.$2.dep_libs)))\
    $(call __libtool_log, $2.libs = $(__libtool_libs.$2.LIBS))\
    $(eval __tmpvar.$2.file_deps := $(call libtool-get-libtool-deps,$(__tmpvar.$2.dep_libs)))\
    $(eval __libtool_libs.$2.DEPS := $(foreach path,$(__tmpvar.$2.file_deps), $(call libtool-name-from-filepath,$(path))))\
    $(call __libtool_log, $2.deps = $(__libtool_libs.$2.DEPS)) \
    $(eval __libtool_libs.processed += $2) \
    $(call __libtool_log, parsed libraries: $(__libtool_libs.processed))\
    $(foreach library,$(__libtool_libs.$2.DEPS), $(call libtool-parse-lib,$(library)))\
    $(eval __libtool_libs.ordered += $2)\
    $(call __libtool_log, ordered list of libraries: $(__libtool_libs.ordered))\
  )

__libtool_log = \
  $(if $(strip $(LIBTOOL_DEBUG)),\
    $(call __libtool_info,$1),\
  )

__libtool_info = $(info LIBTOOL: $1)

libtool-clear-vars = \
  $(foreach lib,$(__libtool_libs.processed),\
    $(eval __libtool_libs.$(lib).LIBS := $(empty))\
    $(eval __libtool_libs.$(lib).STATIC_LIB := $(empty))\
    $(eval __libtool_libs.$(lib).DYN_LIB := $(empty))\
    $(eval __libtool_libs.$(lib).LIBS_SEARCH_PATH := $(empty))\
  )\
  $(eval __libtool_libs.ordered := $(empty))\
  $(eval __libtool_libs.processed := $(empty))\
  $(eval __libtool.link.Lpath := $(empty))\
  $(eval __libtool.link.command := $(empty))\
  $(eval __libtool.link.libs := $(empty))\
  $(eval __libtool.link.shared_libs := $(empty))

libtool-lib-processed = \
  $(findstring ___$1___, $(foreach lib,$(__libtool_libs.processed), ___$(lib)___))

libtool-gen-link-command = \
  $(eval __tmpvar.cmd := $(filter-out -L%,$(__libtool.link.command)))\
  $(eval __tmpvar.cmd := $(filter-out -l%,$(__tmpvar.cmd)))\
  $(eval __tmpvar.cmd += $(__libtool.link.Lpath))\
  $(eval __tmpvar.cmd += $(call libtool-get-libs-search-paths))\
  $(eval __tmpvar.cmd += $(call libtool-get-all-libs))\
  $(eval __tmpvar.cmd += $(__libtool.link.shared_libs))\
  $(call __libtool_log, "Link Command:" $(__tmpvar.cmd))\
  $(__tmpvar.cmd)

libtool-get-libs-search-paths = \
  $(eval __tmpvar.paths := $(empty))\
  $(foreach library,$(__libtool_libs.ordered),\
    $(foreach path,$(__libtool_libs.$(library).LIBS_SEARCH_PATH),\
      $(if $(findstring $(path), $(__tmpvar.paths)), ,\
        $(eval __tmpvar.paths += $(path))\
      )\
    )\
  )\
  $(call __libtool_log, search paths $(__tmpvar.paths))\
  $(strip $(__tmpvar.paths))

libtool-get-all-libs = \
  $(eval __tmpvar.static_libs_reverse := $(empty))\
  $(eval __tmpvar.static_libs := $(empty))\
  $(eval __tmpvar.libs := $(empty))\
  $(foreach library,$(__libtool_libs.ordered),\
    $(eval __tmpvar.static_libs_reverse += $(__libtool_libs.$(library).STATIC_LIB))\
    $(foreach dylib,$(__libtool_libs.$(library).LIBS),\
      $(if $(findstring $(dylib), $(__tmpvar.libs)), ,\
        $(eval __tmpvar.libs += $(dylib))\
      )\
    )\
  )\
  $(foreach path,$(__tmpvar.static_libs_reverse),\
    $(eval __tmpvar.static_libs := $(path) $(__tmpvar.static_libs))\
  )\
  $(strip $(__tmpvar.static_libs) $(__tmpvar.libs) )

libtool-find-lib = \
  $(eval __tmpvar := $(empty))\
  $(foreach path,$(__libtool.link.Lpath),\
    $(eval __tmpvar += $(wildcard $(patsubst -L%,%,$(path))/lib$1.la))\
  ) \
  $(firstword $(__tmpvar))

libtool-name-from-filepath = \
  $(patsubst lib%.la,%,$(notdir $1))

libtool-get-libtool-deps = \
  $(filter %.la,$1)

libtool-get-deps = \
  $(filter %.la,$1)

libtool-get-libs = \
  $(filter -l%,$1)

libtool-get-search-paths = \
  $(filter -L%,$1)

libtool-get-dependency-libs = \
  $(shell $(HOST_SED) -n "s/^dependency_libs='\(.*\)'/\1/p" $1)

libtool-replace-prefixes = \
  $(subst $(BUILD_PREFIX),$(GSTREAMER_ROOT),$1 )

libtool-get-static-library = \
  $(shell $(HOST_SED) -n "s/^old_library='\(.*\)'/\1/p" $1)
