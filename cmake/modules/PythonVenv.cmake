include_guard(GLOBAL)

option(QGC_AUTO_PYTHON_VENV "Auto-create <repo>/tools/.venv with generator deps if missing" ON)

if(DEFINED ENV{QGC_PYTHON_ENV} AND NOT "$ENV{QGC_PYTHON_ENV}" STREQUAL "")
    get_filename_component(_qgc_venv_dir "$ENV{QGC_PYTHON_ENV}" ABSOLUTE)
else()
    set(_qgc_venv_dir "${CMAKE_SOURCE_DIR}/tools/.venv")
endif()

if(CMAKE_HOST_WIN32)
    set(_qgc_venv_python "${_qgc_venv_dir}/Scripts/python.exe")
else()
    set(_qgc_venv_python "${_qgc_venv_dir}/bin/python")
endif()

# Restore generator dependencies if another tooling profile removed them.
function(_qgc_sync_venv_if_stale _py)
    if(NOT QGC_AUTO_PYTHON_VENV)
        return()
    endif()

    execute_process(
        COMMAND "${_py}" "${CMAKE_SOURCE_DIR}/tools/setup/install_python.py" scripts --check
        RESULT_VARIABLE _deps_result
        OUTPUT_QUIET ERROR_QUIET
    )
    if(_deps_result EQUAL 0)
        return()
    endif()
    message(STATUS "QGC: .venv missing generator deps — re-syncing via tools/setup/install_python.py scripts")
    execute_process(
        COMMAND "${_py}" "${CMAKE_SOURCE_DIR}/tools/setup/install_python.py" scripts
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        RESULT_VARIABLE _sync_result
        OUTPUT_VARIABLE _sync_output
        ERROR_VARIABLE _sync_output
    )
    if(NOT _sync_result EQUAL 0)
        message(FATAL_ERROR "QGC: failed to re-sync .venv generator deps (exit ${_sync_result}):\n${_sync_output}\n"
                            "Run manually: python tools/setup/install_python.py scripts"
        )
    endif()
endfunction()

# Pin Python3_EXECUTABLE and Python_EXECUTABLE to the venv: upstream deps (mavlink) call
# find_package(Python), which otherwise picks system Python (3.14 on Windows CI crashes pymavlink mavgen).
macro(_qgc_pin_python _py)
    # FindPython and FindPython3 define these mixed-case cache variables.
    # cmake-lint: disable=C0103
    set(Python3_EXECUTABLE
        "${_py}"
        CACHE FILEPATH "Python interpreter (workspace .venv)" FORCE
    )
    set(Python_EXECUTABLE
        "${_py}"
        CACHE FILEPATH "Python interpreter (workspace .venv)" FORCE
    )
endmacro()

if(DEFINED CACHE{Python3_EXECUTABLE})
    if(NOT EXISTS "${Python3_EXECUTABLE}")
        message(STATUS "QGC: clearing stale cached Python interpreter: ${Python3_EXECUTABLE}")
        unset(Python3_EXECUTABLE CACHE)
        unset(Python_EXECUTABLE CACHE)
    elseif(EXISTS "${_qgc_venv_python}" AND "${Python3_EXECUTABLE}" STREQUAL "${_qgc_venv_python}")
        _qgc_sync_venv_if_stale("${_qgc_venv_python}")
        _qgc_pin_python("${_qgc_venv_python}")
        return()
    else()
        return()
    endif()
endif()

if(EXISTS "${_qgc_venv_dir}" AND NOT EXISTS "${_qgc_venv_python}")
    message(FATAL_ERROR "QGC: ${_qgc_venv_dir} exists but has no interpreter at "
                        "${_qgc_venv_python}. Remove it and reconfigure, or run: "
                        "python tools/setup/install_python.py scripts --environment ${_qgc_venv_dir}"
    )
endif()

if(NOT EXISTS "${_qgc_venv_python}" AND QGC_AUTO_PYTHON_VENV)
    find_program(_qgc_boot_python NAMES python3 python)
    if(NOT _qgc_boot_python)
        message(FATAL_ERROR "QGC: no python3 found to bootstrap .venv. "
                            "Install Python 3.10+ or run tools/setup/install_python.py manually."
        )
    endif()
    execute_process(COMMAND "${_qgc_boot_python}" -c "import sys; sys.exit(0 if sys.version_info >= (3, 10) else 1)"
                    RESULT_VARIABLE _qgc_boot_ok
    )
    if(NOT _qgc_boot_ok EQUAL 0)
        message(FATAL_ERROR "QGC: bootstrap interpreter ${_qgc_boot_python} is older than Python 3.10 "
                            "(required by tools/pyproject.toml). Install Python 3.10+ and reconfigure."
        )
    endif()
    message(STATUS "QGC: .venv missing — creating it via tools/setup/install_python.py scripts")
    execute_process(
        COMMAND "${_qgc_boot_python}" "${CMAKE_SOURCE_DIR}/tools/setup/install_python.py" scripts
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        RESULT_VARIABLE _qgc_venv_result
        OUTPUT_VARIABLE _qgc_venv_output
        ERROR_VARIABLE _qgc_venv_output
    )
    if(NOT _qgc_venv_result EQUAL 0)
        message(FATAL_ERROR "QGC: failed to create .venv (exit ${_qgc_venv_result}):\n${_qgc_venv_output}\n"
                            "Run manually: python tools/setup/install_python.py scripts"
        )
    endif()
endif()

if(EXISTS "${_qgc_venv_python}")
    _qgc_sync_venv_if_stale("${_qgc_venv_python}")
    _qgc_pin_python("${_qgc_venv_python}")
    message(STATUS "QGC: using Python venv interpreter ${Python3_EXECUTABLE}")
else()
    message(WARNING "QGC: no .venv found and QGC_AUTO_PYTHON_VENV=OFF; generators will use system "
                    "python (may lack defusedxml/jinja2). Run: python tools/setup/install_python.py scripts"
    )
endif()
