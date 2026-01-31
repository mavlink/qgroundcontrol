#pragma once

#include "UnitTest.h"

class FTPControllerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testInitialState();
    void _testListDirectory();
    void _testListDirectoryCancel();
    void _testDownloadFile();
    void _testDownloadFileCancel();
    void _testUploadFile();
    void _testUploadFileCancel();
    void _testDeleteFile();
    void _testDeleteFileCancel();
    void _testConcurrentOperationsPrevented();
    void _testInvalidLocalDirectory();
    void _testInvalidLocalFile();
};
