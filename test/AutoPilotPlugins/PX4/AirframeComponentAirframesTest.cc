#include "AirframeComponentAirframesTest.h"

#include <QtTest/QTest>

#include "AirframeComponentAirframes.h"
#include "PX4AirframeLoader.h"

UT_REGISTER_TEST(AirframeComponentAirframesTest, TestLabel::Unit)

void AirframeComponentAirframesTest::_sortedTypesStandardFramesFirst()
{
    // Unit test builds always load the resource based airframe metadata; the
    // load is guarded so repeated calls are no-ops
    PX4AirframeLoader::loadAirframeMetaData();
    QVERIFY2(!AirframeComponentAirframes::get().isEmpty(), "Airframe metadata failed to load");

    const QList<AirframeComponentAirframes::AirframeType_t*> sorted = AirframeComponentAirframes::sortedTypes();
    QCOMPARE(sorted.count(), AirframeComponentAirframes::get().count());

    // The standard airframes show first so users don't have to scroll past the
    // exotic frames to find a regular quad
    const QStringList priorityNames = {
        QStringLiteral("Quadrotor x"),
        QStringLiteral("Standard Plane"),
        QStringLiteral("Standard VTOL"),
    };
    QVERIFY(sorted.count() > priorityNames.count());
    for (int i = 0; i < priorityNames.count(); i++) {
        QCOMPARE(sorted.at(i)->name, priorityNames.at(i));
    }

    // The remaining groups follow in QMap key order (case-sensitive
    // alphabetical) with no duplicates of the priority entries
    for (int i = priorityNames.count(); i < sorted.count(); i++) {
        QVERIFY2(!priorityNames.contains(sorted.at(i)->name),
                 qPrintable(QStringLiteral("Priority group duplicated in remainder: %1").arg(sorted.at(i)->name)));
        if (i > priorityNames.count()) {
            QVERIFY2(sorted.at(i - 1)->name < sorted.at(i)->name,
                     qPrintable(QStringLiteral("Remainder not alphabetical: '%1' before '%2'")
                                    .arg(sorted.at(i - 1)->name, sorted.at(i)->name)));
        }
    }
}

void AirframeComponentAirframesTest::_typeCategory_data()
{
    QTest::addColumn<QString>("group");
    QTest::addColumn<QString>("category");

    QTest::newRow("copter") << QStringLiteral("Quadrotor x") << QStringLiteral("Copter");
    QTest::newRow("plane") << QStringLiteral("Standard Plane") << QStringLiteral("Plane");
    QTest::newRow("vtol") << QStringLiteral("Standard VTOL") << QStringLiteral("VTOL");
    QTest::newRow("rover") << QStringLiteral("Rover") << QStringLiteral("Rover");
    QTest::newRow("other class") << QStringLiteral("Balloon") << QStringLiteral("Other");
    QTest::newRow("mixed classes") << QStringLiteral("Simulation") << QStringLiteral("Other");
}

void AirframeComponentAirframesTest::_typeCategory()
{
    QFETCH(QString, group);
    QFETCH(QString, category);

    PX4AirframeLoader::loadAirframeMetaData();

    const AirframeComponentAirframes::AirframeType_t* type = AirframeComponentAirframes::get().value(group, nullptr);
    QVERIFY2(type, qPrintable(QStringLiteral("Airframe group not found: %1").arg(group)));
    QCOMPARE(type->category, category);
}
