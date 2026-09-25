#include <memory>

#include <QtCore/QObject>
#include <QtCore/QRegularExpression>
#include <QtCore/QStringList>
#include <QtTest/QTest>

#include "GPSNotificationQueue.h"
#include "UnitTest.h"

namespace {

struct QueueOwner : QObject
{
    GPSNotificationQueue notifications{this};
};

}  // namespace

class GPSNotificationQueueTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _synchronousOwnerDeletionIsReported();
};

void GPSNotificationQueueTest::_synchronousOwnerDeletionIsReported()
{
    auto owner = std::make_unique<QueueOwner>();
    QStringList delivered;
    {
        const GPSNotificationQueue::Scope operation(owner->notifications);
        owner->notifications.post(1, [&]() {
            delivered.append(QStringLiteral("delete"));
            owner.reset();
        });
        owner->notifications.post(2, [&]() { delivered.append(QStringLiteral("later")); });
        QVERIFY(delivered.isEmpty());
        expectLogMessage("GPS.Core.GPSNotificationQueue", QtWarningMsg,
                         QRegularExpression(QStringLiteral("^QObject was deleted by its own change notification")));
    }
    verifyExpectedLogMessage();
    QCOMPARE(delivered, QStringList{QStringLiteral("delete")});
}

UT_REGISTER_TEST(GPSNotificationQueueTest, TestLabel::Unit)

#include "GPSNotificationQueueTest.moc"
