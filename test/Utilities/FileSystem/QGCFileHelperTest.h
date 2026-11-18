#pragma once

#include "UnitTest.h"

/// Tests for QGCFileHelper (generic file system utilities)
class QGCFileHelperTest : public UnitTest
{
    Q_OBJECT

private slots:
    // exists() tests
    void _testExistsRegularFile();
    void _testExistsDirectory();
    void _testExistsNonExistent();
    void _testExistsQtResource();

    // ensureDirectoryExists() and ensureParentExists() tests
    void _testEnsureDirectory_data();
    void _testEnsureDirectory();

    // optimalBufferSize() tests
    void _testOptimalBufferSize();

    // atomicWrite() tests
    void _testAtomicWrite_data();
    void _testAtomicWrite();
    void _testAtomicWriteEmptyPath();

    // Disk space utilities tests
    void _testAvailableDiskSpaceBasic();
    void _testAvailableDiskSpaceEmptyPath();
    void _testHasSufficientDiskSpaceBasic();
    void _testHasSufficientDiskSpaceZeroBytes();
    void _testHasSufficientDiskSpaceWithMargin();

    // URL/Path utilities tests
    void _testToLocalPathFromUrl();
    void _testLocalPaths_data();
    void _testLocalPaths();
    void _testIsQtResource();

    // Checksum utilities tests
    void _testComputeHash();
    void _testComputeFileHash();
    void _testVerifyFileHash();
    void _testHashAlgorithmName();

    // Temporary file utilities tests
    void _testTempDirectory();
    void _testUniqueTempPath();
    void _testCreateTempFile();
    void _testCreateTempFileWithTemplate();
    void _testCreateTempCopy();
    void _testReplaceFileFromTemp();
    void _testReplaceFileFromTempWithBackup();
    void _testCopyDirectoryRecursively();
    void _testMoveFileOrCopyFile();
    void _testMoveFileOrCopyDirectory();
    void _testReplaceFileFromTempInvalidArgs();
};
