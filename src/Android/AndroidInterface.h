#pragma once

#include <QtCore/QString>
#include <QtCore/qjnitypes.h>
#include <functional>

Q_DECLARE_JNI_CLASS(QGCActivity, "org/mavlink/qgroundcontrol/QGCActivity")

namespace AndroidInterface {
void setNativeMethods();
bool checkStoragePermissions();
QString getSDCardPath();
void setKeepScreenOn(bool on);
void openFileImportDialog(const QString& destPath, std::function<void(const QString&)> callback);

constexpr const char* kJniQGCActivityClassName = "org/mavlink/qgroundcontrol/QGCActivity";
}  // namespace AndroidInterface
