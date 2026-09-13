# Synclair Vision custom build overrides.
# Keep product-specific defaults out of QGC's upstream-owned CMake files.

set(QGC_APP_NAME "SynclairQGC" CACHE STRING "Application name" FORCE)
set(QGC_STABLE_BUILD ON CACHE BOOL "Stable release build" FORCE)
