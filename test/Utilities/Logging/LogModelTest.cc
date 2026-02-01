#include "LogModelTest.h"
#include "QGCLogEntry.h"
#include "LogModel.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

void LogModelTest::init()
{
    UnitTest::init();
    _model = new LogModel();
}

void LogModelTest::cleanup()
{
    delete _model;
    _model = nullptr;
    UnitTest::cleanup();
}

void LogModelTest::_testInitialState()
{
    QCOMPARE(_model->rowCount(), 0);
    QVERIFY(_model->allFormatted().isEmpty());
}

void LogModelTest::_testAppendEntry()
{
    QGCLogEntry entry;
    entry.timestamp = QDateTime::currentDateTime();
    entry.message = "Test message";
    entry.level = QGCLogEntry::Info;

    QSignalSpy countSpy(_model, &LogModel::countChanged);

    _model->append(entry);

    // Process events to allow queued connection to complete
    QCoreApplication::processEvents();

    QCOMPARE(_model->rowCount(), 1);
    QCOMPARE(countSpy.count(), 1);
}

void LogModelTest::_testAppendMultiple()
{
    for (int i = 0; i < 10; ++i) {
        QGCLogEntry entry;
        entry.timestamp = QDateTime::currentDateTime();
        entry.message = QString("Message %1").arg(i);
        _model->append(entry);
    }

    QCoreApplication::processEvents();

    QCOMPARE(_model->rowCount(), 10);
}

void LogModelTest::_testClear()
{
    // Add some entries
    for (int i = 0; i < 5; ++i) {
        QGCLogEntry entry;
        entry.timestamp = QDateTime::currentDateTime();
        entry.message = QString("Message %1").arg(i);
        _model->append(entry);
    }

    QCoreApplication::processEvents();
    QCOMPARE(_model->rowCount(), 5);

    _model->clear();

    QCOMPARE(_model->rowCount(), 0);
}

void LogModelTest::_testRoleNames()
{
    const auto roles = _model->roleNames();

    QVERIFY(roles.contains(LogModel::TimestampRole));
    QVERIFY(roles.contains(LogModel::LevelRole));
    QVERIFY(roles.contains(LogModel::CategoryRole));
    QVERIFY(roles.contains(LogModel::MessageRole));
    QVERIFY(roles.contains(LogModel::FormattedRole));

    QCOMPARE(roles[LogModel::TimestampRole], QByteArray("timestamp"));
    QCOMPARE(roles[LogModel::LevelRole], QByteArray("level"));
    QCOMPARE(roles[LogModel::CategoryRole], QByteArray("category"));
    QCOMPARE(roles[LogModel::MessageRole], QByteArray("message"));
    QCOMPARE(roles[LogModel::FormattedRole], QByteArray("formatted"));
}

void LogModelTest::_testDataAccess()
{
    QGCLogEntry entry;
    entry.timestamp = QDateTime::fromString("2024-01-15T10:30:00.000", Qt::ISODateWithMs);
    entry.message = "Test message";
    entry.category = "TestCategory";
    entry.level = QGCLogEntry::Warning;

    _model->append(entry);
    QCoreApplication::processEvents();

    const QModelIndex index = _model->index(0);

    QCOMPARE(_model->data(index, LogModel::MessageRole).toString(), QString("Test message"));
    QCOMPARE(_model->data(index, LogModel::CategoryRole).toString(), QString("TestCategory"));
    QCOMPARE(_model->data(index, LogModel::LevelRole).toInt(), static_cast<int>(QGCLogEntry::Warning));

    // Invalid index should return empty
    const QModelIndex invalidIndex = _model->index(-1);
    QVERIFY(!_model->data(invalidIndex, LogModel::MessageRole).isValid());
}

void LogModelTest::_testMaxEntries()
{
    _model->setMaxEntries(5);
    QCOMPARE(_model->maxEntries(), 5);

    // Add more than max entries
    for (int i = 0; i < 10; ++i) {
        QGCLogEntry entry;
        entry.timestamp = QDateTime::currentDateTime();
        entry.message = QString("Message %1").arg(i);
        _model->append(entry);
    }

    QCoreApplication::processEvents();

    // Should be trimmed to max
    QCOMPARE(_model->rowCount(), 5);

    // First entry should be message 5 (oldest removed)
    const QModelIndex firstIndex = _model->index(0);
    QVERIFY(_model->data(firstIndex, LogModel::MessageRole).toString().contains("5"));
}

void LogModelTest::_testAllFormatted()
{
    for (int i = 0; i < 3; ++i) {
        QGCLogEntry entry;
        entry.timestamp = QDateTime::currentDateTime();
        entry.message = QString("Message %1").arg(i);
        _model->append(entry);
    }

    QCoreApplication::processEvents();

    const QStringList formatted = _model->allFormatted();
    QCOMPARE(formatted.size(), 3);

    QVERIFY(formatted[0].contains("Message 0"));
    QVERIFY(formatted[1].contains("Message 1"));
    QVERIFY(formatted[2].contains("Message 2"));
}

void LogModelTest::_testCountChanged()
{
    QSignalSpy countSpy(_model, &LogModel::countChanged);

    QGCLogEntry entry;
    entry.timestamp = QDateTime::currentDateTime();
    entry.message = "Test";

    _model->append(entry);
    QCoreApplication::processEvents();

    QCOMPARE(countSpy.count(), 1);

    _model->clear();
    QCOMPARE(countSpy.count(), 2);
}
