#include "QGCLoggingCategory.h"

// Parser executables retain Qt logging but do not construct the application's QML logging registry.
QGCLoggingCategory::QGCLoggingCategory(const QString&) {}
