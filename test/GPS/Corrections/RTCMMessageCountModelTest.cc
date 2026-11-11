#include "RTCMMessageCountModelTest.h"

#include <QtCore/QList>
#include <QtCore/QPair>
#include <QtTest/QAbstractItemModelTester>
#include <QtTest/QSignalSpy>

#include "RTCMMessageCountModel.h"

void RTCMMessageCountModelTest::_messageCountsChangeInPlace()
{
    using Rows = QList<QPair<int, quint64>>;
    RTCMMessageCountModel model;
    QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);
    QSignalSpy inserted(&model, &QAbstractItemModel::rowsInserted);
    QSignalSpy changed(&model, &QAbstractItemModel::dataChanged);
    QSignalSpy resets(&model, &QAbstractItemModel::modelReset);
    const auto rows = [&model]() {
        Rows result;
        for (int row = 0; row < model.rowCount(); ++row) {
            const QModelIndex index = model.index(row);
            result.append({index.data(RTCMMessageCountModel::MessageIdRole).toInt(),
                           index.data(RTCMMessageCountModel::CountRole).toULongLong()});
        }
        return result;
    };

    // Each new ID inserts one row at its ascending position; ID 0 counts unidentified frames.
    QList<int> insertedAt;
    for (const int messageId : {1077, 1005, 1230, 0}) {
        model.record(messageId, 0);
        insertedAt.append(inserted.last().at(1).toInt());
    }
    QCOMPARE(insertedAt, QList<int>({0, 0, 2, 0}));
    QCOMPARE(rows(), Rows({{0, 1}, {1005, 1}, {1077, 1}, {1230, 1}}));
    QVERIFY(changed.isEmpty());

    // A repeated ID changes only the count and interval of its row.
    model.record(1077, 1000);
    model.record(1077, 2000);
    QCOMPARE(rows(), Rows({{0, 1}, {1005, 1}, {1077, 3}, {1230, 1}}));
    QCOMPARE(inserted.size(), 4);
    QCOMPARE(changed.size(), 2);
    QCOMPARE(changed.last().at(0).toModelIndex(), model.index(2));
    QCOMPARE(changed.last().at(1).toModelIndex(), model.index(2));
    QCOMPARE(changed.last().at(2).value<QList<int>>(),
             QList<int>({RTCMMessageCountModel::CountRole, RTCMMessageCountModel::IntervalRole}));
    QVERIFY(resets.isEmpty());

    model.clear();
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(resets.size(), 1);
    model.clear();
    QCOMPARE(resets.size(), 1);
}

void RTCMMessageCountModelTest::_messageIntervals()
{
    RTCMMessageCountModel model;
    QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);
    QSignalSpy changed(&model, &QAbstractItemModel::dataChanged);
    // Rows ascend by ID: 1005, 1077, 1087, 1097.
    const auto interval = [&model](int row) {
        return model.index(row).data(RTCMMessageCountModel::IntervalRole).toDouble();
    };

    // An interval needs a frame after the first; each ID has its own.
    model.record(1077, 0);
    model.record(1005, 0);
    QCOMPARE(interval(1), 0.);
    model.record(1077, 1000);
    model.record(1077, 2000);
    QCOMPARE(interval(1), 1.);
    QCOMPARE(interval(0), 0.);
    model.record(1005, 10000);
    QCOMPARE(interval(0), 10.);

    // Epochs that one network read delivers together share the time since the epochs before them.
    for (const qint64 receivedAtMs : {0, 0, 2000, 2000, 4000, 4000}) {
        model.record(1087, receivedAtMs);
    }
    QCOMPARE(interval(2), 1.);
    // Frames that arrive together and nothing after them tell no interval.
    model.record(1097, 0);
    model.record(1097, 0);
    QCOMPARE(interval(3), 0.);

    // Only the latest frames count, so a changed rate replaces the old one.
    for (int frame = 1; frame <= RTCMMessageCountModel::INTERVAL_WINDOW; ++frame) {
        model.record(1077, 2000 + frame * 200);
    }
    QCOMPARE(interval(1), 0.2);

    // An ID that stops arriving loses its interval and keeps its count; the others keep theirs.
    const qint64 lastFastFrame = 2000 + RTCMMessageCountModel::INTERVAL_WINDOW * 200;
    changed.clear();
    model.expireIntervals(lastFastFrame + 1500);
    QVERIFY(changed.isEmpty());
    model.expireIntervals(lastFastFrame + 1700);
    QCOMPARE(changed.size(), 1);
    QCOMPARE(changed.last().at(0).toModelIndex(), model.index(1));
    QCOMPARE(changed.last().at(2).value<QList<int>>(), QList<int>{RTCMMessageCountModel::IntervalRole});
    QCOMPARE(interval(1), 0.);
    QCOMPARE(model.index(1).data(RTCMMessageCountModel::CountRole).toULongLong(), quint64(11));
    QCOMPARE(interval(0), 10.);
    // The next frames measure it again.
    model.record(1077, 9000);
    QCOMPARE(interval(1), 0.);
    model.record(1077, 10000);
    QCOMPARE(interval(1), 1.);
}

UT_REGISTER_TEST_LIGHTWEIGHT(RTCMMessageCountModelTest, TestLabel::Unit)
