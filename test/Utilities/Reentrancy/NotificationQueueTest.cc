#include <memory>

#include <QtCore/QObject>
#include <QtCore/QRegularExpression>
#include <QtCore/QStringList>
#include <QtTest/QTest>

#include "NotificationQueue.h"
#include "UnitTest.h"

namespace {

struct QueueOwner : QObject
{
    NotificationQueue notifications{this};
};

}  // namespace

class NotificationQueueTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _synchronousOwnerDeletionIsReported();
};

void NotificationQueueTest::_synchronousOwnerDeletionIsReported()
{
    auto owner = std::make_unique<QueueOwner>();
    QStringList delivered;
    {
        const NotificationQueue::Scope operation(owner->notifications);
        owner->notifications.post(1, [&]() {
            delivered.append(QStringLiteral("delete"));
            owner.reset();
        });
        owner->notifications.post(2, [&]() { delivered.append(QStringLiteral("later")); });
        QVERIFY(delivered.isEmpty());
        expectLogMessage("Utilities.NotificationQueue", QtWarningMsg,
                         QRegularExpression(QStringLiteral("^QObject was deleted by its own change notification")));
    }
    verifyExpectedLogMessage();
    QCOMPARE(delivered, QStringList{QStringLiteral("delete")});
}

UT_REGISTER_TEST(NotificationQueueTest, TestLabel::Unit)

#include "NotificationQueueTest.moc"
