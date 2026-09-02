#include "SettingsManagerTest.h"

#include "SettingsManager.h"
#include "Viewer3DSettings.h"

#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>

UT_REGISTER_TEST(SettingsManagerTest, TestLabel::Unit)

void SettingsManagerTest::_registerCustomGroup()
{
    SettingsManager settingsManager;
    auto *group = new Viewer3DSettings(nullptr);

    settingsManager.registerCustomSettingsGroup(QStringLiteral("myCustomSettings"), group);

    QVERIFY(settingsManager.contains(QStringLiteral("myCustomSettings")));
    QCOMPARE(settingsManager.value(QStringLiteral("myCustomSettings")).value<QObject*>(), group);
    QCOMPARE(group->parent(), &settingsManager);
}

void SettingsManagerTest::_registerRejectsBuiltInAccessorCollision()
{
    SettingsManager settingsManager;
    QPointer<Viewer3DSettings> group = new Viewer3DSettings(nullptr);

    expectLogMessage("Utilities.SettingsManager", QtWarningMsg, QRegularExpression(QStringLiteral("collides with a built-in settings group")));
    settingsManager.registerCustomSettingsGroup(QStringLiteral("appSettings"), group);
    verifyExpectedLogMessage();

    QVERIFY(!settingsManager.contains(QStringLiteral("appSettings")));
    QVERIFY(group.isNull()); // Rejected group is deleted
}

void SettingsManagerTest::_registerRejectsDuplicateAccessor()
{
    SettingsManager settingsManager;
    auto *firstGroup = new Viewer3DSettings(nullptr);
    QPointer<Viewer3DSettings> secondGroup = new Viewer3DSettings(nullptr);

    settingsManager.registerCustomSettingsGroup(QStringLiteral("myCustomSettings"), firstGroup);

    expectLogMessage("Utilities.SettingsManager", QtWarningMsg, QRegularExpression(QStringLiteral("already registered")));
    settingsManager.registerCustomSettingsGroup(QStringLiteral("myCustomSettings"), secondGroup);
    verifyExpectedLogMessage();

    QCOMPARE(settingsManager.value(QStringLiteral("myCustomSettings")).value<QObject*>(), firstGroup);
    QVERIFY(secondGroup.isNull()); // Rejected group is deleted
}

void SettingsManagerTest::_registerSamePointerTwiceKeepsGroupAlive()
{
    SettingsManager settingsManager;
    QPointer<Viewer3DSettings> group = new Viewer3DSettings(nullptr);

    settingsManager.registerCustomSettingsGroup(QStringLiteral("myCustomSettings"), group);

    expectLogMessage("Utilities.SettingsManager", QtWarningMsg, QRegularExpression(QStringLiteral("already registered")));
    settingsManager.registerCustomSettingsGroup(QStringLiteral("myCustomSettings"), group);
    verifyExpectedLogMessage();

    QVERIFY(!group.isNull()); // The stored group must not be deleted
    QCOMPARE(settingsManager.value(QStringLiteral("myCustomSettings")).value<QObject*>(), group.data());
}

void SettingsManagerTest::_registerRejectsInvalidArguments()
{
    SettingsManager settingsManager;
    QPointer<Viewer3DSettings> group = new Viewer3DSettings(nullptr);

    expectLogMessage("Utilities.SettingsManager", QtWarningMsg, QRegularExpression(QStringLiteral("invalid accessor name or null group")));
    settingsManager.registerCustomSettingsGroup(QString(), group);
    verifyExpectedLogMessage();
    QVERIFY(group.isNull()); // Rejected group is deleted

    expectLogMessage("Utilities.SettingsManager", QtWarningMsg, QRegularExpression(QStringLiteral("invalid accessor name or null group")));
    settingsManager.registerCustomSettingsGroup(QStringLiteral("myCustomSettings"), nullptr);
    verifyExpectedLogMessage();

    QVERIFY(!settingsManager.contains(QStringLiteral("myCustomSettings")));
}

void SettingsManagerTest::_registerRejectsInvalidQmlIdentifier()
{
    SettingsManager settingsManager;

    const QStringList badAccessors = {
        QStringLiteral("3DSettings"),
        QStringLiteral("my-settings"),
        QStringLiteral("MySettings"),
        QStringLiteral("my settings"),
    };
    for (const QString &accessor : badAccessors) {
        QPointer<Viewer3DSettings> group = new Viewer3DSettings(nullptr);
        expectLogMessage("Utilities.SettingsManager", QtWarningMsg, QRegularExpression(QStringLiteral("invalid accessor name or null group")));
        settingsManager.registerCustomSettingsGroup(accessor, group);
        verifyExpectedLogMessage();
        QVERIFY2(!settingsManager.contains(accessor), qPrintable(accessor));
        QVERIFY2(group.isNull(), qPrintable(accessor)); // Rejected group is deleted
    }
}
