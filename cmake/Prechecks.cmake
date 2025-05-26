set(QT_DEFAULT_MAJOR_VERSION 6)

find_program(QMAKE_EXECUTABLE
    NAMES qmake6
    HINTS ${QT_HOST_PATH} ${QT_ROOT_DIR} ${QTDIR}
    ENV QTDIR
    PATH_SUFFIXES bin
)

if(NOT QMAKE_EXECUTABLE)
    message(FATAL_ERROR "qmake6 not found. Please set QT_ROOT_DIR or QTDIR correctly.")
endif()

execute_process(
    COMMAND "${QMAKE_EXECUTABLE}" -query QT_VERSION
    OUTPUT_VARIABLE QT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE _qmake_query_res
)
if(_qmake_query_res)
    message(FATAL_ERROR "Failed to run ${QMAKE_EXECUTABLE} -query QT_VERSION")
endif()

if(QT_VERSION VERSION_LESS "${QGC_QT_MINIMUM_VERSION}")
    message(FATAL_ERROR
        "Qt version too old: need ≥ ${QGC_QT_MINIMUM_VERSION}, "
        "found ${QT_VERSION}"
    )
elseif(QT_VERSION VERSION_GREATER "${QGC_QT_MAXIMUM_VERSION}")
    message(FATAL_ERROR
        "Qt version too new: need ≤ ${QGC_QT_MAXIMUM_VERSION}, "
        "found ${QT_VERSION}"
    )
endif()
