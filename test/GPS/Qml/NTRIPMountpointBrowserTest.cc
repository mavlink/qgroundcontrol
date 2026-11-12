#include "NTRIPMountpointBrowserTest.h"

#include <memory>

#include <QtPositioning/QGeoCoordinate>
#include <QtQuick/QQuickItem>
#include <QtTest/QSignalSpy>

#include "Fact.h"
#include "Fixtures/RAIIFixtures.h"
#include "ManualScheduler.h"
#include "MultiVehicleManager.h"
#include "NTRIP/Support/ScriptedNTRIPCaster.h"
#include "NTRIPManager.h"
#include "NTRIPSourceTable.h"
#include "NTRIPSourceTableController.h"
#include "QmlTesting/QmlFixtures.h"
#include "SettingsManager.h"
#include "UnitsSettings.h"

using namespace GPSTest;

namespace {

/// A source-table row of the mountpoint @a name, which the caster describes as @a town; one without a valid
/// @a position has no coordinates.
QString tableRow(const QString& name, const QString& town, const QGeoCoordinate& position,
                 const QString& format = QStringLiteral("RTCM 3.2"))
{
    const auto degrees = [&position](double value) {
        return position.isValid() ? QString::number(value, 'f', 7) : QString();
    };
    return QStringLiteral("STR;%1;%2;%3;1005(10),1077(1);2;GPS;NET;CHE;%4;%5;0;0;gen;none;B;N;4800;\r\n")
        .arg(name, town, format, degrees(position.latitude()), degrees(position.longitude()));
}

/// The mountpoint list over @a model inside @a window; null on failure, with the reason in lastError().
std::unique_ptr<QObject> createList(TestFixtures::QmlEngine& engine, const TestFixtures::QmlWindowFixture& window,
                                    QAbstractItemModel& model)
{
    return engine.create(TestFixtures::sourceQmlUrl(QStringLiteral("GPS/Qml/NTRIPMountpointList.qml")),
                         {{QStringLiteral("parent"), QVariant::fromValue(window.contentItem())},
                          {QStringLiteral("width"), 480},
                          {QStringLiteral("height"), 300},
                          {QStringLiteral("model"), QVariant::fromValue(&model)}});
}

/// The mountpoint browser of @a manager inside @a window, for a connection that @a enabled turns on and a ground
/// station at @a gcsPosition; null on failure, with the reason in lastError().
std::unique_ptr<QObject> createBrowser(TestFixtures::QmlEngine& engine, const TestFixtures::QmlWindowFixture& window,
                                       NTRIPManager& manager, Fact& enabled, const QGeoCoordinate& gcsPosition)
{
    return engine.create(TestFixtures::sourceQmlUrl(QStringLiteral("GPS/Qml/NTRIPMountpointBrowser.qml")),
                         {{QStringLiteral("parent"), QVariant::fromValue(window.contentItem())},
                          {QStringLiteral("width"), 600},
                          {QStringLiteral("enabledFact"), QVariant::fromValue(&enabled)},
                          {QStringLiteral("ntripManager"), QVariant::fromValue(&manager)},
                          {QStringLiteral("gcsPosition"), QVariant::fromValue(gcsPosition)}});
}

/// The row the list shows for @a mountpoint; null while it has none.
QQuickItem* listRow(QObject* list, const QString& mountpoint)
{
    const auto rows = TestFixtures::findItems(qobject_cast<QQuickItem*>(list), [&mountpoint](QQuickItem* item) {
        return item->property("mountpoint").toString() == mountpoint;
    });
    return rows.size() == 1 ? rows.constFirst() : nullptr;
}

/// What the part @a name of @a item reads; empty while the part is hidden or missing.
QString shownText(const QObject* item, const char* name)
{
    const auto* const part = item ? item->findChild<QQuickItem*>(QString::fromLatin1(name)) : nullptr;
    return part && part->isVisible() ? part->property("text").toString() : QString();
}

QStringList mountpointNames(const QAbstractItemModel& rows)
{
    QStringList names;
    for (int row = 0; row < rows.rowCount(); ++row) {
        names.append(rows.index(row, 0).data(NTRIPSourceTableModel::MountpointRole).toString());
    }
    return names;
}

/// A manager whose caster is @a caster, with the connection off, as the browser needs it.
void useCaster(NTRIPManager& manager, const ScriptedNTRIPCaster& caster)
{
    NTRIPManager::Configuration configuration;
    configuration.connection = caster.connectionConfig(QString());
    manager.setConfiguration(configuration);
}

/// Presses Browse in @a browser; false while the button cannot be pressed.
bool pressBrowse(QObject* browser)
{
    auto* const button = browser->findChild<QObject*>(QStringLiteral("ntripBrowseButton"));
    return button && button->property("enabled").toBool() && QMetaObject::invokeMethod(button, "click");
}

/// Presses Browse in @a browser, to which @a caster answers with @a table. False unless the table is then listed.
bool browse(QObject* browser, NTRIPManager& manager, ScriptedNTRIPCaster& caster, const QString& table)
{
    if (!pressBrowse(browser) || !caster.serveSourceTable(table + QStringLiteral("ENDSOURCETABLE\r\n"))) {
        return false;
    }
    return QTest::qWaitFor(
        [&manager] {
            return manager.sourceTableController()->fetchStatus() == NTRIPSourceTableController::FetchStatus::Success;
        },
        TestTimeout::mediumMs());
}

}  // namespace

void NTRIPMountpointBrowserTest::_rowShowsDistanceInTheDistanceUnit()
{
    TestFixtures::SettingsFixture saved;
    Fact* const units = SettingsManager::instance()->unitsSettings()->horizontalDistanceUnits();
    saved.setFactValue(units, UnitsSettings::HorizontalDistanceUnitsMeters);
    const QGeoCoordinate reference(47.0, 8.0);
    const QGeoCoordinate far = reference.atDistanceAndAzimuth(2500, 90);
    NTRIPSourceTableModel model;
    model.parseSourceTable(
        tableRow(QStringLiteral("NEAR"), QStringLiteral("Near"), reference.atDistanceAndAzimuth(300, 90)) +
            tableRow(QStringLiteral("FAR"), QStringLiteral("Far"), far) +
            tableRow(QStringLiteral("UNPLACED"), QStringLiteral("Unplaced"), QGeoCoordinate()),
        reference);
    TestFixtures::QmlWindowFixture window(640, 400);
    TestFixtures::QmlEngine engine;
    const std::unique_ptr<QObject> list = createList(engine, window, model);
    QVERIFY2(list, qPrintable(engine.lastError()));
    QVERIFY(window.show());
    const auto distance = [&list](const char* mountpoint) {
        return shownText(listRow(list.get(), QString::fromLatin1(mountpoint)), "ntripMountpointDistance");
    };
    QTRY_VERIFY_WITH_TIMEOUT(listRow(list.get(), QStringLiteral("UNPLACED")), TestTimeout::mediumMs());

    // Whole meters or feet, and kilometers or miles from one of those on, as the base station's distance is shown.
    QCOMPARE(distance("NEAR"), QStringLiteral("300 m"));
    QCOMPARE(distance("FAR"), QStringLiteral("2.5 km"));
    // A mountpoint the caster gives no coordinates for has no distance.
    QVERIFY(distance("UNPLACED").isEmpty());

    // The distances follow the unit setting while they are shown, and the position they are measured from.
    units->setRawValue(UnitsSettings::HorizontalDistanceUnitsFeet);
    QCOMPARE(distance("NEAR"), QStringLiteral("984 ft"));
    QCOMPARE(distance("FAR"), QStringLiteral("1.6 mi"));
    model.updateDistances(far);
    QCOMPARE(distance("FAR"), QStringLiteral("0 ft"));
}

void NTRIPMountpointBrowserTest::_rowSaysWhyAMountpointCannotWork()
{
    NTRIPSourceTableModel model;
    model.parseSourceTable(
        tableRow(QStringLiteral("USABLE"), QStringLiteral("Town"), QGeoCoordinate()) +
        tableRow(QStringLiteral("OTHER"), QStringLiteral("Town"), QGeoCoordinate(), QStringLiteral("CMR+")));
    TestFixtures::QmlWindowFixture window(640, 400);
    TestFixtures::QmlEngine engine;
    const std::unique_ptr<QObject> list = createList(engine, window, model);
    QVERIFY2(list, qPrintable(engine.lastError()));
    QSignalSpy selections(list.get(), SIGNAL(mountpointSelected(QString)));
    QVERIFY(window.show());
    QTRY_VERIFY_WITH_TIMEOUT(listRow(list.get(), QStringLiteral("OTHER")), TestTimeout::mediumMs());
    QQuickItem* const usable = listRow(list.get(), QStringLiteral("USABLE"));
    QQuickItem* const other = listRow(list.get(), QStringLiteral("OTHER"));
    QVERIFY(usable && other);

    // Every row says what the mountpoint asks for and sends.
    for (const QQuickItem* row : {usable, other}) {
        QCOMPARE(shownText(row, "ntripMountpointNotes"), QStringLiteral("Needs a login · Messages 1005(10),1077(1)"));
    }
    QVERIFY(shownText(usable, "ntripMountpointProblem").isEmpty());
    QCOMPARE(shownText(other, "ntripMountpointProblem"),
             QStringLiteral("Listed as CMR+: only RTCM 3 corrections can be used."));

    // The row stays, and can be selected: the caster's table may be wrong.
    const auto buttons =
        TestFixtures::findItems(other, [](QQuickItem* item) { return item->inherits("QQuickAbstractButton"); });
    QCOMPARE(buttons.size(), 1);
    QVERIFY(buttons.constFirst()->isEnabled());
    QVERIFY(QMetaObject::invokeMethod(buttons.constFirst(), "click"));
    QCOMPARE(selections.size(), 1);
    QCOMPARE(selections.constFirst().constFirst().toString(), QStringLiteral("OTHER"));
}

void NTRIPMountpointBrowserTest::_browseMeasuresFromTheGroundStation()
{
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(SettingsManager::instance()->unitsSettings()->horizontalDistanceUnits(),
                       UnitsSettings::HorizontalDistanceUnitsMeters);
    QVERIFY(!MultiVehicleManager::instance()->activeVehicle());
    ScriptedNTRIPCaster caster;
    QVERIFY(caster.isListening());
    ManualScheduler scheduler;
    NTRIPManager manager(nullptr, &scheduler);
    useCaster(manager, caster);
    Fact enabled(0, QStringLiteral("enabled"), FactMetaData::valueTypeBool);
    enabled.setRawValue(false);
    const QGeoCoordinate berlin(52.5, 13.4);
    const QGeoCoordinate potsdam = berlin.atDistanceAndAzimuth(25000, 250);
    TestFixtures::QmlWindowFixture window(640, 600);
    TestFixtures::QmlEngine engine;
    const std::unique_ptr<QObject> browser = createBrowser(engine, window, manager, enabled, potsdam);
    QVERIFY2(browser, qPrintable(engine.lastError()));
    QVERIFY(window.show());

    // No vehicle is connected, so the ground station is where the distances are measured from.
    const QString table = tableRow(QStringLiteral("NEWYORK"), QStringLiteral("New York"), QGeoCoordinate(40.7, -74.0)) +
                          tableRow(QStringLiteral("BERLIN"), QStringLiteral("Berlin"), berlin) +
                          tableRow(QStringLiteral("POTSDAM"), QStringLiteral("Potsdam"), potsdam);
    QVERIFY(browse(browser.get(), manager, caster, table));
    const QAbstractItemModel* const model = manager.sourceTableController()->mountpointModel();
    QCOMPARE(mountpointNames(*model),
             (QStringList{QStringLiteral("POTSDAM"), QStringLiteral("BERLIN"), QStringLiteral("NEWYORK")}));
    auto* const list = browser->findChild<QObject*>(QStringLiteral("ntripMountpointList"));
    QVERIFY(list);
    QTRY_VERIFY_WITH_TIMEOUT(listRow(list, QStringLiteral("BERLIN")), TestTimeout::mediumMs());
    QCOMPARE(shownText(listRow(list, QStringLiteral("BERLIN")), "ntripMountpointDistance"), QStringLiteral("25.0 km"));

    // Without a position of the ground station either, the list keeps the caster's order and shows no distances.
    // The table is still fresh, so the caster is not asked again.
    QVERIFY(browser->setProperty("gcsPosition", QVariant::fromValue(QGeoCoordinate())));
    QVERIFY(pressBrowse(browser.get()));
    QCOMPARE(mountpointNames(*model),
             (QStringList{QStringLiteral("NEWYORK"), QStringLiteral("BERLIN"), QStringLiteral("POTSDAM")}));
    QTRY_VERIFY_WITH_TIMEOUT(listRow(list, QStringLiteral("BERLIN")), TestTimeout::mediumMs());
    QVERIFY(shownText(listRow(list, QStringLiteral("BERLIN")), "ntripMountpointDistance").isEmpty());
}

void NTRIPMountpointBrowserTest::_filterNarrowsTheList()
{
    ScriptedNTRIPCaster caster;
    QVERIFY(caster.isListening());
    ManualScheduler scheduler;
    NTRIPManager manager(nullptr, &scheduler);
    useCaster(manager, caster);
    Fact enabled(0, QStringLiteral("enabled"), FactMetaData::valueTypeBool);
    enabled.setRawValue(false);
    TestFixtures::QmlWindowFixture window(640, 600);
    TestFixtures::QmlEngine engine;
    const std::unique_ptr<QObject> browser = createBrowser(engine, window, manager, enabled, QGeoCoordinate());
    QVERIFY2(browser, qPrintable(engine.lastError()));
    QVERIFY(window.show());
    auto* const list = browser->findChild<QQuickItem*>(QStringLiteral("ntripMountpointList"));
    auto* const filter = browser->findChild<QQuickItem*>(QStringLiteral("ntripMountpointFilter"));
    QVERIFY(list && filter);
    const auto count = [&browser] { return shownText(browser.get(), "ntripMountpointFilterCount"); };

    // There is nothing to filter before the caster's mountpoints are listed.
    QVERIFY(!filter->isVisible());
    QVERIFY(browse(browser.get(), manager, caster,
                   tableRow(QStringLiteral("ZH_MSM7"), QStringLiteral("Zurich"), QGeoCoordinate()) +
                       tableRow(QStringLiteral("BE_MSM7"), QStringLiteral("Bern"), QGeoCoordinate()) +
                       tableRow(QStringLiteral("BE_MSM4"), QStringLiteral("Bern"), QGeoCoordinate())));
    QVERIFY(filter->isVisible());
    QCOMPARE(list->property("count").toInt(), 3);
    QVERIFY(count().isEmpty());

    // The name, or the caster's description of the mountpoint. The list tells its new length when it next updates.
    QVERIFY(filter->setProperty("text", QStringLiteral("msm7")));
    QCOMPARE(manager.sourceTableController()->mountpointFilter(), QStringLiteral("msm7"));
    QCOMPARE(list->property("count").toInt(), 2);
    QTRY_COMPARE_WITH_TIMEOUT(count(), QStringLiteral("2 of 3 mountpoints"), TestTimeout::mediumMs());
    QVERIFY(filter->setProperty("text", QStringLiteral("zurich")));
    QCOMPARE(list->property("count").toInt(), 1);
    QTRY_COMPARE_WITH_TIMEOUT(count(), QStringLiteral("1 of 3 mountpoints"), TestTimeout::mediumMs());

    // A filter that leaves nothing hides the list, and stays to be changed.
    QVERIFY(filter->setProperty("text", QStringLiteral("geneva")));
    QTRY_COMPARE_WITH_TIMEOUT(count(), QStringLiteral("0 of 3 mountpoints"), TestTimeout::mediumMs());
    QVERIFY(!list->isVisible());
    QVERIFY(filter->isVisible());
    QVERIFY(filter->setProperty("text", QString()));
    QVERIFY(count().isEmpty());
    QTRY_VERIFY_WITH_TIMEOUT(list->isVisible(), TestTimeout::mediumMs());
    QCOMPARE(list->property("count").toInt(), 3);
}

UT_REGISTER_TEST(NTRIPMountpointBrowserTest, TestLabel::Unit)
