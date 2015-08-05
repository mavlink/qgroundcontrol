# set the QGC version from git

exists ($$PWD/.git) {
  GIT_DESCRIBE = $$system(git --git-dir $$PWD/.git --work-tree $$PWD describe --always --tags)
  WindowsBuild {
    QGC_GIT_VER = echo extern \"C\" { const char *git_version() { return \"$$GIT_DESCRIBE\"; } } > git_version.cpp
    LIBS += git_version.obj
  } else {
    QGC_GIT_VER = echo \"extern \\\"C\\\" { const char *git_version() { return \\\"$$GIT_DESCRIBE\\\"; } }\" > git_version.cpp
    LIBS += git_version.o
  }
}

WindowsBuild {
  LIBS += git_version.obj
} else {
  LIBS += git_version.o
}

CONFIG(debug) {
  GIT_VERSION_CXXFLAGS = $$QMAKE_CXXFLAGS_DEBUG
} else {
  GIT_VERSION_CXXFLAGS = $$QMAKE_CXXFLAGS_RELEASE
}

QMAKE_PRE_LINK += $$QGC_GIT_VER && $$QMAKE_CXX -c $$GIT_VERSION_CXXFLAGS git_version.cpp
