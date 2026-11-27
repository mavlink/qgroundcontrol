include_guard(GLOBAL)

include(CheckCompilerFlag)
include(CheckLinkerFlag)
include(CMakePushCheckState)

option(QGC_ENABLE_HARDENING "Apply security hardening flags to non-Debug builds" ON)
option(QGC_REPRODUCIBLE_BUILD "Strip build paths from non-Debug binaries (-ffile-prefix-map)" ON)

# Sets <out_var> to whether every architecture the build targets matches <pattern>.
function(_qgc_target_architectures_match pattern out_var)
    if(CMAKE_OSX_ARCHITECTURES)
        set(_architectures ${CMAKE_OSX_ARCHITECTURES})
    else()
        set(_architectures ${CMAKE_SYSTEM_PROCESSOR})
    endif()

    set(_all_match TRUE)
    if(NOT _architectures)
        set(_all_match FALSE)
    endif()
    foreach(_architecture IN LISTS _architectures)
        if(NOT _architecture MATCHES "${pattern}")
            set(_all_match FALSE)
        endif()
    endforeach()
    set(${out_var}
        ${_all_match}
        PARENT_SCOPE
    )
endfunction()

# Appends <flag> to the list <out_var> if the C++ compiler takes it.
# cmake-lint: disable=C0103
function(_qgc_try_compile_flag flag out_var)
    string(MAKE_C_IDENTIFIER "QGC_HARDEN_CXX_${flag}" _probe)
    cmake_push_check_state(RESET)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(CMAKE_REQUIRED_FLAGS "-Werror=unused-command-line-argument")
    endif()
    check_compiler_flag(CXX "${flag}" ${_probe})
    cmake_pop_check_state()
    if(${_probe})
        set(_acc ${${out_var}})
        list(APPEND _acc "${flag}")
        set(${out_var}
            "${_acc}"
            PARENT_SCOPE
        )
    endif()
endfunction()

# Appends <flag>, scoped to the slice <arch> of a universal build, to the list <out_var> if the C++ compiler takes it
# for that architecture.
# cmake-lint: disable=C0103
function(_qgc_try_compile_flag_arch flag arch out_var)
    string(MAKE_C_IDENTIFIER "QGC_HARDEN_CXX_${arch}_${flag}" _probe)
    cmake_push_check_state(RESET)
    set(CMAKE_OSX_ARCHITECTURES "${arch}")
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(CMAKE_REQUIRED_FLAGS "-Werror=unused-command-line-argument")
    endif()
    check_compiler_flag(CXX "${flag}" ${_probe})
    cmake_pop_check_state()
    if(${_probe})
        set(_acc ${${out_var}})
        list(APPEND _acc "-Xarch_${arch}" "${flag}")
        set(${out_var}
            "${_acc}"
            PARENT_SCOPE
        )
    endif()
endfunction()

# Appends <flag> to the list <out_var> if the linker takes it.
function(_qgc_try_link_flag flag out_var)
    string(MAKE_C_IDENTIFIER "QGC_HARDEN_LD_${flag}" _probe)
    cmake_push_check_state(RESET)
    check_linker_flag(CXX "${flag}" ${_probe})
    cmake_pop_check_state()
    if(${_probe})
        set(_acc ${${out_var}})
        list(APPEND _acc "${flag}")
        set(${out_var}
            "${_acc}"
            PARENT_SCOPE
        )
    endif()
endfunction()

# Adds the hardening and reproducible-build flags that the toolchain takes to every non-Debug target, and records them
# in QGC_HARDENING_CXX_FLAGS and QGC_HARDENING_LINK_FLAGS, which the configure summary and the contract tests read
# under those public names. One function, because the flags of each platform are chosen in one place.
# cmake-lint: disable=C0103,R0912,R0915
function(qgc_apply_global_hardening)
    list(APPEND CMAKE_MESSAGE_CONTEXT hardening)
    set(_cxx "")
    set(_link "")

    if(QGC_ENABLE_HARDENING)
        if(MSVC)
            _qgc_try_compile_flag("/guard:cf" _cxx)
            _qgc_try_link_flag("/guard:cf" _link)
            _qgc_try_link_flag("/DYNAMICBASE" _link)
            _qgc_try_link_flag("/HIGHENTROPYVA" _link)
            _qgc_try_link_flag("/NXCOMPAT" _link)
        else()
            list(APPEND _cxx "-U_FORTIFY_SOURCE" "-D_FORTIFY_SOURCE=3")
            _qgc_try_compile_flag("-fstack-protector-strong" _cxx)
            _qgc_try_compile_flag("-fstack-clash-protection" _cxx)
            # "standard" adds pac-ret to BTI. On macOS arm64 an exception thrown through code built with pac-ret is
            # not caught: the program ends (AppleClang 17). Apple systems get BTI alone.
            if(APPLE)
                set(_qgc_branch_protection "-mbranch-protection=bti")
            else()
                set(_qgc_branch_protection "-mbranch-protection=standard")
            endif()
            list(LENGTH CMAKE_OSX_ARCHITECTURES _qgc_osx_arch_count)
            if(APPLE AND _qgc_osx_arch_count GREATER 1)
                # Scope architecture-specific flags to their universal-build slice.
                foreach(_arch IN LISTS CMAKE_OSX_ARCHITECTURES)
                    if(_arch MATCHES "^x86_64")
                        _qgc_try_compile_flag_arch("-fcf-protection=full" "${_arch}" _cxx)
                    elseif(_arch MATCHES "^arm64")
                        _qgc_try_compile_flag_arch("${_qgc_branch_protection}" "${_arch}" _cxx)
                    endif()
                endforeach()
            else()
                _qgc_target_architectures_match("^(x86_64h|x86_64|AMD64|i[0-9]86|x86)$" _qgc_all_x86)
                _qgc_target_architectures_match("^(aarch64|arm64)$" _qgc_all_aarch64)
                if(_qgc_all_x86)
                    _qgc_try_compile_flag("-fcf-protection=full" _cxx)
                elseif(_qgc_all_aarch64)
                    _qgc_try_compile_flag("${_qgc_branch_protection}" _cxx)
                endif()
            endif()
            _qgc_try_link_flag("-Wl,-z,relro" _link)
            _qgc_try_link_flag("-Wl,-z,now" _link)
            _qgc_try_link_flag("-Wl,-z,noexecstack" _link)
        endif()
    endif()

    if(QGC_REPRODUCIBLE_BUILD AND NOT MSVC)
        _qgc_try_compile_flag("-ffile-prefix-map=${CMAKE_SOURCE_DIR}=." _cxx)
        _qgc_try_compile_flag("-ffile-prefix-map=${CMAKE_BINARY_DIR}=./build" _cxx)
        if(CPM_SOURCE_CACHE)
            _qgc_try_compile_flag("-ffile-prefix-map=${CPM_SOURCE_CACHE}=./.cpm" _cxx)
        endif()
    endif()

    foreach(_flag IN LISTS _cxx)
        add_compile_options("$<$<NOT:$<CONFIG:Debug>>:${_flag}>")
    endforeach()
    foreach(_flag IN LISTS _link)
        add_link_options("$<$<NOT:$<CONFIG:Debug>>:${_flag}>")
    endforeach()

    set(QGC_HARDENING_CXX_FLAGS
        "${_cxx}"
        CACHE INTERNAL "Applied hardening/reproducible compile flags"
    )
    set(QGC_HARDENING_LINK_FLAGS
        "${_link}"
        CACHE INTERNAL "Applied hardening link flags"
    )

    list(LENGTH _cxx _n_cxx)
    list(LENGTH _link _n_link)
    message(STATUS "QGC: Hardening — ${_n_cxx} compile flag(s), ${_n_link} link flag(s) (non-Debug configs)")
endfunction()
