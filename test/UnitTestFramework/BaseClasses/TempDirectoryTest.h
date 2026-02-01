#pragma once

#include <QtCore/QTemporaryDir>

#include <memory>

#include "UnitTest.h"

/// Test fixture providing automatic temporary directory management.
///
/// Creates a temporary directory before each test and cleans it up after.
/// The directory is guaranteed to be valid and writable during test execution.
///
/// Example:
/// @code
/// class MyFileTest : public TempDirectoryTest
/// {
///     Q_OBJECT
/// private slots:
///     void _testFileOperations() {
///         QString testFile = tempPath("test.txt");
///         QFile file(testFile);
///         file.open(QIODevice::WriteOnly);
///         // ... test file operations ...
///     }
/// };
/// @endcode
class TempDirectoryTest : public UnitTest
{
    Q_OBJECT

public:
    explicit TempDirectoryTest(QObject* parent = nullptr) : UnitTest(parent) {}

    /// Returns the path to the temporary directory
    QString tempDirPath() const
    {
        return _tempDir ? _tempDir->path() : QString();
    }

    /// Returns a path within the temporary directory
    /// @param relativePath Path relative to temp directory (e.g., "subdir/file.txt")
    QString tempPath(const QString& relativePath) const
    {
        return _tempDir ? _tempDir->filePath(relativePath) : QString();
    }

    /// Returns the QTemporaryDir instance
    QTemporaryDir* tempDir() const
    {
        return _tempDir.get();
    }

    /// Creates a subdirectory within the temp directory
    /// @param subdir Subdirectory name
    /// @return Full path to created subdirectory, empty on failure
    QString createSubDir(const QString& subdir)
    {
        if (!_tempDir) {
            return QString();
        }
        QDir dir(_tempDir->path());
        if (dir.mkpath(subdir)) {
            return dir.filePath(subdir);
        }
        return QString();
    }

    /// Creates a file with the given content in the temp directory
    /// @param relativePath Path relative to temp directory
    /// @param content File content
    /// @return true on success
    bool createFile(const QString& relativePath, const QByteArray& content)
    {
        if (!_tempDir) {
            return false;
        }
        QFile file(tempPath(relativePath));
        if (!file.open(QIODevice::WriteOnly)) {
            return false;
        }
        return file.write(content) == content.size();
    }

    /// Creates a file with string content
    bool createFile(const QString& relativePath, const QString& content)
    {
        return createFile(relativePath, content.toUtf8());
    }

protected slots:
    void init() override
    {
        UnitTest::init();
        _tempDir = std::make_unique<QTemporaryDir>();
        QVERIFY2(_tempDir->isValid(), "Failed to create temporary directory");
    }

    void cleanup() override
    {
        _tempDir.reset();
        UnitTest::cleanup();
    }

private:
    std::unique_ptr<QTemporaryDir> _tempDir;
};
