#pragma once

#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(AndroidJavaLog)

namespace AndroidLogSink {

/**
 * Registers the JNI native method for QGCNativeLogSink.nativeLog().
 * Must be called from JNI_OnLoad (or equivalent) after the class loader
 * is available.
 */
void setNativeMethods();

} // namespace AndroidLogSink
