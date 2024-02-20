#pragma once

#include <QtCore/QString>

class AndroidInterface
{
public:
    static void cleanJavaException();
    static const char* getQGCActivityClassName();
    static bool checkStoragePermissions();
    static QString getSDCardPath();
};
