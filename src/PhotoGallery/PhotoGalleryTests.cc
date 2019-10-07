/****************************************************************************
 *
 *   (c) 2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "PhotoFileStore.h"

#include <QtTest/QtTest>

#include <fcntl.h>
#include <netinet/ip.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>

/* Test helpers. */

namespace {

void
setFileContents(const QString & path, const QByteArray & contents)
{
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadWrite | QIODevice::NewOnly));
    QCOMPARE(contents.size(), file.write(contents));
    file.close();
}

void
verifyFileContents(const QString & path, const QByteArray & contents)
{
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    auto data = file.readAll();
    QCOMPARE(data, contents);
    file.close();
}

void
verifyFileMissing(const QString & path)
{
    QFile file(path);
    QVERIFY(!file.open(QIODevice::ReadOnly));
}

}  // namespace

/* Actual tests. */

class PhotoGalleryTests : public QObject {
    Q_OBJECT

private slots:
    void testPhotoFileStoreFilesystem();
    void testPhotoFileStoreNameCollisions();
    void testPhotoFileStoreInitNotifications();
    void testPhotoFileStoreRuntimeNotifications();
};

/// Verify photo store interacts correctly with filesystem.
void PhotoGalleryTests::testPhotoFileStoreFilesystem()
{
    QTemporaryDir temp_dir;
    QVERIFY(temp_dir.isValid());

    setFileContents(temp_dir.filePath("foo.jpg"), "foo_content");
    setFileContents(temp_dir.filePath("bar.jpg"), "bar_content");
    PhotoFileStore store(temp_dir.path());

    QCOMPARE(std::set<QString>({"bar.jpg", "foo.jpg"}), store.ids());

    auto c = store.read("bar.jpg");
    QVERIFY(c.canConvert<QByteArray>());
    QCOMPARE(c.value<QByteArray>(), "bar_content");

    auto id = store.add("baz.jpg", "baz_content");
    QCOMPARE(id, "baz.jpg");
    QCOMPARE(std::set<QString>({"bar.jpg", "baz.jpg", "foo.jpg"}), store.ids());

    store.remove({"foo.jpg"});
    QCOMPARE(std::set<QString>({"bar.jpg", "baz.jpg"}), store.ids());
    verifyFileContents(temp_dir.filePath("bar.jpg"), "bar_content");
    verifyFileContents(temp_dir.filePath("baz.jpg"), "baz_content");
    verifyFileMissing(temp_dir.filePath("foo.jpg"));
}

/// Verify that names are kept if possible, but collisions are resolved.
void PhotoGalleryTests::testPhotoFileStoreNameCollisions()
{
    QTemporaryDir temp_dir;
    QVERIFY(temp_dir.isValid());

    PhotoFileStore store(temp_dir.path());

    auto foo_jpg = store.add("foo.jpg", "foo1");
    QCOMPARE(foo_jpg, "foo.jpg");
    auto foo2_jpg = store.add("foo.jpg", "foo2");
    QVERIFY(foo2_jpg != foo_jpg);

    QCOMPARE(std::set<QString>({foo_jpg, foo2_jpg}), store.ids());
    QVERIFY(QRegularExpression("^.*\\.jpg$").match(foo2_jpg).hasMatch());
    verifyFileContents(temp_dir.filePath(foo2_jpg), "foo2");
}

/// Verify observer is modified when store path is reconfigured.
///
/// This situation arises in the main program as the correct path is configured
/// at runtime, after all models and views have been instantiated already.
void PhotoGalleryTests::testPhotoFileStoreInitNotifications()
{
    PhotoFileStore store;

    std::set<QString> added;
    QObject::connect(
        &store, &PhotoFileStore::added,
        [&added](const std::set<QString> & ids) {
            added.insert(ids.begin(), ids.end());
        });
    QCOMPARE(std::set<QString>(), added);

    QTemporaryDir temp_dir;
    QVERIFY(temp_dir.isValid());

    setFileContents(temp_dir.filePath("foo.jpg"), "foo_content");
    setFileContents(temp_dir.filePath("bar.jpg"), "bar_content");

    store.setLocation(temp_dir.path());
    QCOMPARE(std::set<QString>({"bar.jpg", "foo.jpg"}), store.ids());
}

/// Verify that notifications are sent when changing store.
void PhotoGalleryTests::testPhotoFileStoreRuntimeNotifications()
{
    QTemporaryDir temp_dir;
    QVERIFY(temp_dir.isValid());

    PhotoFileStore store(temp_dir.path());
    std::set<QString> added;
    std::set<QString> removed;
    QObject::connect(
        &store, &PhotoFileStore::added,
        [&added](const std::set<QString> & ids) {
            added.insert(ids.begin(), ids.end());
        });
    QObject::connect(
        &store, &PhotoFileStore::removed,
        [&removed](const std::set<QString> & ids) {
            removed.insert(ids.begin(), ids.end());
        });

    store.add("foo.jpg", "xxx");
    QCOMPARE(std::set<QString>({"foo.jpg"}), added);
    QCOMPARE(std::set<QString>(), removed);
    added.clear();

    store.remove({"foo.jpg"});
    QCOMPARE(std::set<QString>(), added);
    QCOMPARE(std::set<QString>({"foo.jpg"}), removed);

}

QTEST_MAIN(PhotoGalleryTests)
#include "PhotoGalleryTests.moc"
