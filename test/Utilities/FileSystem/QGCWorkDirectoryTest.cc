#include "QGCWorkDirectoryTest.h"

#include <memory>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
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

UT_REGISTER_TEST(QGCWorkDirectoryTest, TestLabel::Unit, TestLabel::Utilities)
