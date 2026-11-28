#include "QGCWorkDirectoryTest.h"

#include <chrono>
#include <filesystem>
#include <memory>
#include <system_error>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QLockFile>
#include <QtCore/QScopeGuard>
#include <QtCore/QTemporaryDir>

#include "FileFixtures.h"
#include "QGCWorkDirectory.h"

namespace {

const QString kWorkDirectoryTestPrefix = QStringLiteral(".qgc-test-");

QStringList workDirectoriesIn(const QString& parentPath)
{
    return QDir(parentPath)
        .entryList({kWorkDirectoryTestPrefix + QLatin1Char('*')}, QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot,
                   QDir::Name);
}

}  // namespace

// A work directory is created under its parent with the prefix, holds its lock while it exists, and goes with the
// object, with all in it
void QGCWorkDirectoryTest::_testLifetime()
{
    QTemporaryDir parent;
    QVERIFY(parent.isValid());

    QString path;
    {
        const QGCWorkDirectory workDirectory(parent.path(), kWorkDirectoryTestPrefix);
        QVERIFY2(workDirectory.isValid(), qPrintable(workDirectory.errorString()));
        path = workDirectory.path();
        QCOMPARE(QFileInfo(path).absolutePath(), QFileInfo(parent.path()).absoluteFilePath());
        QVERIFY(QFileInfo(path).fileName().startsWith(kWorkDirectoryTestPrefix));
        QVERIFY(QDir(path).mkdir(QStringLiteral("data")));
        QVERIFY(TestFixtures::writeFile(QDir(path).filePath(QStringLiteral("data/file.bin")), "data"));

        // The directory of an operation that runs is not touched
        QCOMPARE(QGCWorkDirectory::removeStale(parent.path(), kWorkDirectoryTestPrefix), 0);
        QVERIFY(QFileInfo::exists(QDir(path).filePath(QStringLiteral("data/file.bin"))));
    }
    QVERIFY(!QFileInfo::exists(path));
    QVERIFY(QDir(parent.path()).isEmpty(QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot));
}

// Directories with the prefix whose lock is not held are removed: one without a lock file, and one whose lock belongs
// to a process that has ended. One whose lock is held, here by this process as by another QGC, stays, and so does a
// directory of another name
void QGCWorkDirectoryTest::_testRemoveStale()
{
    QTemporaryDir parent;
    QVERIFY(parent.isValid());
    const QDir parentDir(parent.path());

    const QString withoutLock = kWorkDirectoryTestPrefix + QStringLiteral("without-lock");
    QVERIFY(parentDir.mkdir(withoutLock));
    QVERIFY(TestFixtures::writeFile(parentDir.filePath(withoutLock + QStringLiteral("/data.bin")), "left"));

    // As QLockFile writes it: process ID, application name, host name, host ID and boot ID. No process has this ID
    const QString endedOwner = kWorkDirectoryTestPrefix + QStringLiteral("ended-owner");
    QVERIFY(parentDir.mkdir(endedOwner));
    QVERIFY(TestFixtures::writeFile(parentDir.filePath(endedOwner + QStringLiteral("/data.bin")), "left"));
    QVERIFY(
        TestFixtures::writeFile(parentDir.filePath(endedOwner + QStringLiteral("/.lock")), "2147483646\nqgc\n\n\n\n"));

    const QString otherName = QStringLiteral("qgc-other");
    QVERIFY(parentDir.mkdir(otherName));

    const auto held = std::make_unique<QGCWorkDirectory>(parent.path(), kWorkDirectoryTestPrefix);
    QVERIFY(held->isValid());

    QCOMPARE(QGCWorkDirectory::removeStale(parent.path(), kWorkDirectoryTestPrefix), 2);
    QCOMPARE(workDirectoriesIn(parent.path()), QStringList{QFileInfo(held->path()).fileName()});
    QVERIFY(QFileInfo(parentDir.filePath(otherName)).isDir());
}

// A link with the prefix is no work directory: what it leads to is neither locked nor emptied
void QGCWorkDirectoryTest::_testRemoveStaleLeavesLinks()
{
    QTemporaryDir parent;
    QTemporaryDir outside;
    QVERIFY(parent.isValid());
    QVERIFY(outside.isValid());
    QVERIFY(TestFixtures::writeFile(outside.filePath(QStringLiteral("kept.bin")), "kept"));

    const QString linkPath = QDir(parent.path()).filePath(kWorkDirectoryTestPrefix + QStringLiteral("link"));
    if (!TestFixtures::createSymbolicLink(outside.path(), linkPath)) {
        QSKIP("This system does not let the test create a symbolic link");
    }
    // QTemporaryDir cannot remove a link to a directory on Windows
    const auto removeLink = qScopeGuard([&linkPath]() {
        std::error_code error;
        std::filesystem::remove(QFileInfo(linkPath).filesystemAbsoluteFilePath(), error);
    });

    QCOMPARE(QGCWorkDirectory::removeStale(parent.path(), kWorkDirectoryTestPrefix), 0);
    QCOMPARE(QDir(outside.path()).entryList(QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot),
             QStringList{QStringLiteral("kept.bin")});
    QVERIFY(QFileInfo(linkPath).isSymLink());
}

// Another QGC that creates a work directory, or looks for stale ones, holds the lock of the parent. While it does,
// nothing is looked for, as a directory that is being created is not locked yet. A directory is created all the same
void QGCWorkDirectoryTest::_testParentLocked()
{
    QTemporaryDir parent;
    QVERIFY(parent.isValid());
    const QDir parentDir(parent.path());
    const QString stale = kWorkDirectoryTestPrefix + QStringLiteral("without-lock");
    QVERIFY(parentDir.mkdir(stale));

    // Held by this process, as by another: a lock is stale only once its process has ended
    QLockFile parentLock(parentDir.filePath(kWorkDirectoryTestPrefix + QStringLiteral("lock")));
    parentLock.setStaleLockTime(std::chrono::milliseconds::zero());
    QVERIFY(parentLock.tryLock(std::chrono::milliseconds::zero()));

    constexpr std::chrono::milliseconds noWait{0};
    QCOMPARE(QGCWorkDirectory::removeStale(parent.path(), kWorkDirectoryTestPrefix, noWait), 0);
    QCOMPARE(workDirectoriesIn(parent.path()), QStringList{stale});

    const QGCWorkDirectory created(parent.path(), kWorkDirectoryTestPrefix, noWait);
    QVERIFY2(created.isValid(), qPrintable(created.errorString()));
    QVERIFY(QFileInfo(created.path()).isDir());

    // Once the other has let go, what was left behind is removed, and the directory in use stays
    parentLock.unlock();
    QCOMPARE(QGCWorkDirectory::removeStale(parent.path(), kWorkDirectoryTestPrefix), 1);
    QCOMPARE(workDirectoriesIn(parent.path()), QStringList{QFileInfo(created.path()).fileName()});
}

// A work directory that cannot be created is not valid and says why. It has no path, so nothing is removed with it
void QGCWorkDirectoryTest::_testParentMissing()
{
    QTemporaryDir root;
    QVERIFY(root.isValid());
    {
        const QGCWorkDirectory workDirectory(root.filePath(QStringLiteral("missing")), kWorkDirectoryTestPrefix);
        QVERIFY(!workDirectory.isValid());
        QVERIFY(workDirectory.path().isEmpty());
        QVERIFY(!workDirectory.errorString().isEmpty());
        QCOMPARE(QGCWorkDirectory::removeStale(root.filePath(QStringLiteral("missing")), kWorkDirectoryTestPrefix), 0);
    }
    QVERIFY(QDir(root.path()).isEmpty(QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot));
}

UT_REGISTER_TEST(QGCWorkDirectoryTest, TestLabel::Unit, TestLabel::Utilities)
