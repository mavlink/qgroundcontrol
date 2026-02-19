#pragma once

#include <QtCore/QString>
#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(AndroidInterfaceLog)

namespace AndroidInterface
{
    void setNativeMethods();
    bool checkStoragePermissions();
    QString getSDCardPath();
    void setKeepScreenOn(bool on);

    constexpr const char *kJniQGCActivityClassName = "org/mavlink/qgroundcontrol/QGCActivity";
};
