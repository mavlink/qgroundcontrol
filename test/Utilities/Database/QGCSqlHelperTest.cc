#include "QGCSqlHelperTest.h"

#include <QtCore/QFileInfo>
#include <QtCore/QTemporaryDir>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtTest/QTest>

#include "QGCSqlHelper.h"
#include "UnitTestList.h"

void QGCSqlHelperTest::_escapeLikePattern()
{
    // No special chars — unchanged
    QCOMPARE(QGCSqlHelper::escapeLikePattern(QStringLiteral("hello")), QStringLiteral("hello"));

    // Percent
    QCOMPARE(QGCSqlHelper::escapeLikePattern(QStringLiteral("100%")), QStringLiteral("100\\%"));

    // Underscore
    QCOMPARE(QGCSqlHelper::escapeLikePattern(QStringLiteral("a_b")), QStringLiteral("a\\_b"));

    // Backslash
    QCOMPARE(QGCSqlHelper::escapeLikePattern(QStringLiteral("c:\\path")), QStringLiteral("c:\\\\path"));

    // All three combined
    QCOMPARE(QGCSqlHelper::escapeLikePattern(QStringLiteral("a%b_c\\d")), QStringLiteral("a\\%b\\_c\\\\d"));

    // Empty string
    QCOMPARE(QGCSqlHelper::escapeLikePattern(QString()), QString());
}

void QGCSqlHelperTest::_scopedConnectionOpen()
{
    QTemporaryDir tempDir;
    const QString dbPath = tempDir.filePath(QStringLiteral("test.db"));

    {
        QGCSqlHelper::ScopedConnection conn(dbPath);
        QVERIFY(conn.isValid());

        QSqlDatabase db = conn.database();
        QVERIFY(db.isOpen());

        // Verify we can execute queries
        QSqlQuery q(db);
        QVERIFY(q.exec(QStringLiteral("CREATE TABLE t (id INTEGER PRIMARY KEY)")));
        QVERIFY(q.exec(QStringLiteral("INSERT INTO t VALUES (1)")));
        QVERIFY(q.exec(QStringLiteral("SELECT id FROM t")));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toInt(), 1);
    }

    // After scope exit, connection should be cleaned up
    // (QSqlDatabase::connectionNames() should not contain the connection)
}

void QGCSqlHelperTest::_scopedConnectionReadOnly()
{
    QTemporaryDir tempDir;
    const QString dbPath = tempDir.filePath(QStringLiteral("readonly.db"));

    // Create a database with data first
    {
        QGCSqlHelper::ScopedConnection writeConn(dbPath);
        QVERIFY(writeConn.isValid());
        QSqlQuery q(writeConn.database());
        QVERIFY(q.exec(QStringLiteral("CREATE TABLE t (v TEXT)")));
        QVERIFY(q.exec(QStringLiteral("INSERT INTO t VALUES ('data')")));
    }

    // Open read-only and verify reads work
    {
        QGCSqlHelper::ScopedConnection readConn(dbPath, true);
        QVERIFY(readConn.isValid());

        QSqlQuery q(readConn.database());
        QVERIFY(q.exec(QStringLiteral("SELECT v FROM t")));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toString(), QStringLiteral("data"));
    }
}

void QGCSqlHelperTest::_scopedConnectionInvalidPath()
{
    // Empty path
    {
        const QString empty;
        QGCSqlHelper::ScopedConnection conn(empty);
        QVERIFY(!conn.isValid());
    }
}

void QGCSqlHelperTest::_scopedConnectionUniqueNames()
{
    QTemporaryDir tempDir;
    const QString dbPath = tempDir.filePath(QStringLiteral("unique.db"));

    // Open two connections simultaneously — should not conflict
    QGCSqlHelper::ScopedConnection conn1(dbPath);
    QGCSqlHelper::ScopedConnection conn2(dbPath, true);

    QVERIFY(conn1.isValid());
    QVERIFY(conn2.isValid());

    // Both should be usable independently
    QSqlQuery q1(conn1.database());
    QVERIFY(q1.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS t (id INTEGER)")));

    QSqlQuery q2(conn2.database());
    QVERIFY(q2.exec(QStringLiteral("SELECT COUNT(*) FROM t")));
}

void QGCSqlHelperTest::_scopedConnectionCleanup()
{
    QTemporaryDir tempDir;
    const QString dbPath = tempDir.filePath(QStringLiteral("cleanup.db"));
    const int beforeCount = QSqlDatabase::connectionNames().size();

    {
        QGCSqlHelper::ScopedConnection conn(dbPath);
        QVERIFY(conn.isValid());
        QCOMPARE(QSqlDatabase::connectionNames().size(), beforeCount + 1);
    }

    // Connection removed after destruction
    QCOMPARE(QSqlDatabase::connectionNames().size(), beforeCount);
}

void QGCSqlHelperTest::_applySqlitePragmas()
{
    QTemporaryDir tempDir;
    const QString dbPath = tempDir.filePath(QStringLiteral("pragma.db"));

    // ScopedConnection applies pragmas automatically, verify them
    QGCSqlHelper::ScopedConnection conn(dbPath);
    QVERIFY(conn.isValid());

    QSqlQuery q(conn.database());

    QVERIFY(q.exec(QStringLiteral("PRAGMA journal_mode")));
    QVERIFY(q.next());
    QCOMPARE(q.value(0).toString().toLower(), QStringLiteral("wal"));

    QVERIFY(q.exec(QStringLiteral("PRAGMA synchronous")));
    QVERIFY(q.next());
    // NORMAL = 1
    QCOMPARE(q.value(0).toInt(), 1);
}

// A transaction larger than the limit leaves a log of its size, which SQLite would keep for as long as the database is
// open: the next write cuts it down
void QGCSqlHelperTest::_writeAheadLogShrinksAfterLargeTransaction()
{
    QTemporaryDir tempDir;
    const QString dbPath = tempDir.filePath(QStringLiteral("log.db"));
    const QString logPath = dbPath + QStringLiteral("-wal");

    QGCSqlHelper::ScopedConnection conn(dbPath);
    QVERIFY(conn.isValid());
    QSqlQuery q(conn.database());
    QVERIFY(q.exec(QStringLiteral("CREATE TABLE Blobs (data BLOB)")));

    // 1.5 times the limit, in the transaction of one statement
    constexpr qint64 kBlobSize = 64 * 1024;
    const qint64 blobCount = (QGCSqlHelper::kWriteAheadLogSizeLimit * 3) / (kBlobSize * 2);
    QVERIFY(q.exec(QStringLiteral("WITH RECURSIVE counter(i) AS (SELECT 1 UNION ALL SELECT i + 1 FROM counter "
                                  "WHERE i < %1) INSERT INTO Blobs SELECT randomblob(%2) FROM counter")
                       .arg(blobCount)
                       .arg(kBlobSize)));
    QCOMPARE_GT(QFileInfo(logPath).size(), QGCSqlHelper::kWriteAheadLogSizeLimit);

    QVERIFY(q.exec(QStringLiteral("INSERT INTO Blobs VALUES (x'00')")));
    QCOMPARE_LE(QFileInfo(logPath).size(), QGCSqlHelper::kWriteAheadLogSizeLimit);
}

UT_REGISTER_TEST(QGCSqlHelperTest, TestLabel::Unit, TestLabel::Utilities)
