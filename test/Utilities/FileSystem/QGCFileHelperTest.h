#pragma once

#include "UnitTest.h"

#include <QtCore/QTemporaryDir>

/// Tests for QGCFileHelper (generic file system utilities)
class QGCFileHelperTest : public UnitTest
{
    Q_OBJECT

public:
    QGCFileHelperTest() = default;

private slots:
    void init() override;
    void cleanup() override;

    // exists() tests
    void _testExistsRegularFile();
    void _testExistsDirectory();
    void _testExistsNonExistent();
    void _testExistsQtResource();

    // ensureDirectoryExists() tests
    void _testEnsureDirectoryExistsAlreadyExists();
    void _testEnsureDirectoryExistsCreate();
    void _testEnsureDirectoryExistsNested();

    // ensureParentExists() tests
    void _testEnsureParentExistsAlreadyExists();
    void _testEnsureParentExistsCreate();
    void _testEnsureParentExistsNested();

    // optimalBufferSize() tests
    void _testOptimalBufferSizeConstants();
    void _testOptimalBufferSizeWithinBounds();
    void _testOptimalBufferSizeCached();
    void _testOptimalBufferSizeWithPath();

    // atomicWrite() tests
    void _testAtomicWriteBasic();
    void _testAtomicWriteOverwrite();
    void _testAtomicWriteCreatesParent();
    void _testAtomicWriteEmptyPath();
    void _testAtomicWriteEmptyData();

    // Disk space utilities tests
    void _testAvailableDiskSpaceBasic();
    void _testAvailableDiskSpaceEmptyPath();
    void _testHasSufficientDiskSpaceBasic();
    void _testHasSufficientDiskSpaceZeroBytes();
    void _testHasSufficientDiskSpaceWithMargin();

    // URL/Path utilities tests
    void _testToLocalPathPlainPaths();
    void _testToLocalPathFileUrls();
    void _testToLocalPathQrcUrls();
    void _testIsLocalPath();
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

private:
    QTemporaryDir *_tempDir = nullptr;
};
