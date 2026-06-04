# Pin the workspace .venv as the interpreter for all Python3 code generators.
# find_package(Python3) otherwise resolves to system python, which lacks the
# generator deps (defusedxml, jinja2, httpx) declared in tools/pyproject.toml,
# producing ModuleNotFoundError during the build. The justfile pins this via
# -DPython3_EXECUTABLE; doing it here makes IDE/raw-cmake configures work too.

option(QGC_AUTO_PYTHON_VENV "Auto-create <repo>/.venv with generator deps if missing" ON)

# Respect an interpreter already pinned by the caller (e.g. justfile, CI).
if(DEFINED CACHE{Python3_EXECUTABLE})
    return()
endif()

if(WIN32)
    set(_qgc_venv_python "${CMAKE_SOURCE_DIR}/.venv/Scripts/python.exe")
else()
    set(_qgc_venv_python "${CMAKE_SOURCE_DIR}/.venv/bin/python")
endif()

# A .venv directory with a missing interpreter is corrupt: install_python.py
# early-returns on an existing dir, so auto-create can't repair it. Fail loud.
if(EXISTS "${CMAKE_SOURCE_DIR}/.venv" AND NOT EXISTS "${_qgc_venv_python}")
    message(FATAL_ERROR "QGC: ${CMAKE_SOURCE_DIR}/.venv exists but has no interpreter at "
                        "${_qgc_venv_python}. Remove it and reconfigure, or run: "
                        "rm -rf .venv && python tools/setup/install_python.py scripts")
endif()

if(NOT EXISTS "${_qgc_venv_python}" AND QGC_AUTO_PYTHON_VENV)
    # Bootstrap with find_program (not find_package) to avoid polluting the
    # Python3_* cache that the real find_package(Python3) calls rely on.
    find_program(_qgc_boot_python NAMES python3 python)
    if(NOT _qgc_boot_python)
        message(FATAL_ERROR "QGC: no python3 found to bootstrap .venv. "
                            "Install Python 3.12+ or run tools/setup/install_python.py manually.")
    endif()
    # tools/pyproject.toml requires >=3.12; reject older interpreters up front so
    # the failure is actionable instead of a confusing downstream install error.
    execute_process(
        COMMAND "${_qgc_boot_python}" -c
                "import sys; sys.exit(0 if sys.version_info >= (3, 12) else 1)"
        RESULT_VARIABLE _qgc_boot_ok
    )
    if(NOT _qgc_boot_ok EQUAL 0)
        message(FATAL_ERROR "QGC: bootstrap interpreter ${_qgc_boot_python} is older than Python 3.12 "
                            "(required by tools/pyproject.toml). Install Python 3.12+ and reconfigure.")
    endif()
    message(STATUS "QGC: .venv missing — creating it via tools/setup/install_python.py scripts")
    execute_process(
        COMMAND "${_qgc_boot_python}" "${CMAKE_SOURCE_DIR}/tools/setup/install_python.py" scripts
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        RESULT_VARIABLE _qgc_venv_result
        OUTPUT_VARIABLE _qgc_venv_output
        ERROR_VARIABLE  _qgc_venv_output
    )
    if(NOT _qgc_venv_result EQUAL 0)
        message(FATAL_ERROR "QGC: failed to create .venv (exit ${_qgc_venv_result}):\n${_qgc_venv_output}\n"
                            "Run manually: python tools/setup/install_python.py scripts")
    endif()
endif()

if(EXISTS "${_qgc_venv_python}")
    set(Python3_EXECUTABLE "${_qgc_venv_python}" CACHE FILEPATH "Python interpreter (workspace .venv)" FORCE)
    message(STATUS "QGC: using Python venv interpreter ${Python3_EXECUTABLE}")
else()
    message(WARNING "QGC: no .venv found and QGC_AUTO_PYTHON_VENV=OFF; generators will use system "
                    "python (may lack defusedxml/jinja2). Run: python tools/setup/install_python.py scripts")
endif()
