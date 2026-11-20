include_guard(GLOBAL)

option(QGC_AUTO_PYTHON_VENV "Auto-create <repo>/tools/.venv with generator deps if missing" ON)

# The Python that uv builds the environment on, and the minimum for any other interpreter.
file(STRINGS "${CMAKE_CURRENT_LIST_DIR}/../../tools/.python-version" QGC_PYTHON_VERSION LIMIT_COUNT 1)

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

# Create or repair the environment. uv runs the installer on the Python named in tools/.python-version,
# downloading it when the host has none, so the host's own python3 is never needed.
function(_qgc_sync_venv)
    find_program(_qgc_uv NAMES uv NO_CACHE)
    if(NOT _qgc_uv)
        message(
            FATAL_ERROR
                "QGC: uv was not found. It creates the Python ${QGC_PYTHON_VERSION} environment that "
                "the code generators run in (${_qgc_venv_dir}). Install it from "
                "https://docs.astral.sh/uv/getting-started/installation/ and reconfigure."
        )
    endif()
    # The installer looks for uv on PATH, which need not include the directory CMake found it in.
    get_filename_component(_qgc_uv_dir "${_qgc_uv}" DIRECTORY)
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env --modify "PATH=path_list_prepend:${_qgc_uv_dir}" "${_qgc_uv}" run --no-project
                --project tools python tools/setup/install_python.py scripts
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        RESULT_VARIABLE _sync_result
        OUTPUT_VARIABLE _sync_output
        ERROR_VARIABLE _sync_output
    )
    if(NOT _sync_result EQUAL 0)
        message(
            FATAL_ERROR
                "QGC: failed to set up ${_qgc_venv_dir} (exit ${_sync_result}):\n${_sync_output}\n"
                "Run from the repository root: "
                "uv run --no-project --project tools python tools/setup/install_python.py scripts"
        )
    endif()
endfunction()

# Restore generator dependencies if another tooling profile removed them, and rebuild an environment
# whose Python predates tools/pyproject.toml's requires-python.
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
    message(STATUS "QGC: .venv is out of date — re-syncing via tools/setup/install_python.py scripts")
    _qgc_sync_venv()
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
    message(
        FATAL_ERROR "QGC: ${_qgc_venv_dir} exists but has no interpreter at "
                    "${_qgc_venv_python}. Remove it and reconfigure."
    )
endif()

if(NOT EXISTS "${_qgc_venv_python}" AND QGC_AUTO_PYTHON_VENV)
    message(STATUS "QGC: .venv missing — creating it via tools/setup/install_python.py scripts")
    _qgc_sync_venv()
endif()

if(EXISTS "${_qgc_venv_python}")
    _qgc_sync_venv_if_stale("${_qgc_venv_python}")
    _qgc_pin_python("${_qgc_venv_python}")
    message(STATUS "QGC: using Python venv interpreter ${Python3_EXECUTABLE}")
else()
    message(
        WARNING
            "QGC: no .venv found and QGC_AUTO_PYTHON_VENV=OFF; generators will use a system "
            "Python ${QGC_PYTHON_VERSION} or newer (may lack defusedxml/jinja2). Run from the repository "
            "root: uv run --no-project --project tools python tools/setup/install_python.py scripts"
    )
endif()
