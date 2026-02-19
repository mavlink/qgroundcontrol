#include "QGCMapEngineManagerArchiveTest.h"

#include <QtCore/QFile>
#include <QtTest/QSignalSpy>

#include "QGCMapEngineManager.h"
#include "UnitTest.h"

void QGCMapEngineManagerArchiveTest::_importArchiveRejectsInvalidInput()
{
    QGCMapEngineManager manager;

    QVERIFY(!manager.importArchive(QString()));
    QVERIFY(manager.errorMessage().contains(QStringLiteral("No archive path specified")));

    QVERIFY(!manager.importArchive(QStringLiteral("/nonexistent/archive.zip")));
    QVERIFY(manager.errorMessage().contains(QStringLiteral("Archive file not found")));

    const QString plainFile = tempPath(QStringLiteral("qgc_map_manager_plain.txt"));
    QFile file(plainFile);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write("not an archive");
    file.close();

    QVERIFY(!manager.importArchive(plainFile));
    QVERIFY(manager.errorMessage().contains(QStringLiteral("Not a supported archive format")));

    QFile::remove(plainFile);
}

void QGCMapEngineManagerArchiveTest::_importArchiveNoTileDatabase()
{
    QGCMapEngineManager manager;

    QSignalSpy actionSpy(&manager, &QGCMapEngineManager::importActionChanged);
    QVERIFY(actionSpy.isValid());

    QVERIFY(manager.importArchive(QStringLiteral(":/unittest/manifest.json.zip")));

    QTRY_COMPARE_WITH_TIMEOUT(manager.importAction(), QGCMapEngineManager::ImportAction::ActionDone,
                              TestTimeout::mediumMs());
    QVERIFY(manager.errorMessage().contains(QStringLiteral("No tile database found in archive")));
}

void QGCMapEngineManagerArchiveTest::_importArchiveRejectsWhenAlreadyImporting()
{
    QGCMapEngineManager manager;
    manager.setImportAction(QGCMapEngineManager::ImportAction::ActionImporting);

    QVERIFY(!manager.importArchive(QStringLiteral(":/unittest/manifest.json.zip")));
    QVERIFY(manager.errorMessage().contains(QStringLiteral("Import already in progress")));
}

UT_REGISTER_TEST(QGCMapEngineManagerArchiveTest, TestLabel::Unit)
