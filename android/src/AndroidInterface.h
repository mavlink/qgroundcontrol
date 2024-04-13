#pragma once

#include <QtCore/QString>
#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(AndroidInterfaceLog);

class AndroidInterface
{
public:
    static void cleanJavaException();
    static const char* getQGCActivityClassName();
    static bool checkStoragePermissions();
    static QString getSDCardPath();
};
