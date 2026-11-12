#include "GPSSatelliteViewTest.h"

#include <chrono>
#include <memory>
#include <vector>

#include <QtCore/QCoreApplication>
#include <QtCore/QEvent>
#include <QtGui/QColor>
#include <QtQuick/QQuickItem>
#include <QtQuickTest/quicktest.h>

#include "Fact.h"
#include "GPSReceiver.h"
#include "GPSReceiverFactGroup.h"
#include "GPSReceiverReports.h"
#include "GPSSatelliteModel.h"
#include "ManualScheduler.h"
#include "QGCPalette.h"
#include "QmlTesting/QmlFixtures.h"
#include "Receiver/Support/GPSReceiverTestSupport.h"
#include "Receiver/Support/ScriptedReceiverWorker.h"

using namespace GPSTest;

namespace {

using Use = GPSSatellite::Use;

GPSSatellite satellite(GPSConstellation constellation, int id, int elevation, int azimuth, int signal, Use use)
{
    return {.constellation = constellation,
            .id = static_cast<uint16_t>(id),
            .azimuthDegrees = static_cast<uint16_t>(azimuth),
            .elevationDegrees = static_cast<int8_t>(elevation),
            .directionKnown = true,
            .signalDbHz = static_cast<uint8_t>(signal),
            .use = use};
}

GPSSatelliteReport reportOf(const std::vector<GPSSatellite>& satellites)
{
    GPSSatelliteReport report;
    report.timestampUs = 1;
    report.inView = static_cast<int>(satellites.size());
    for (const GPSSatellite& listed : satellites) {
        report.addSatellite(listed);
    }
    return report;
}

/// Five constellations: satellites overhead, on the horizon to the north and to the east, one the receiver has no
/// direction of, and an SBAS satellite, whose number has three digits.
std::vector<GPSSatellite> knownSky()
{
    GPSSatellite untracked{.constellation = GPSConstellation::GPS, .id = 12, .use = Use::Unused};
    return {satellite(GPSConstellation::GPS, 2, 90, 0, 44, Use::Used),
            satellite(GPSConstellation::GPS, 5, 0, 0, 50, Use::Used),
            untracked,
            satellite(GPSConstellation::GPS, 25, 0, 90, 25, Use::Unused),
            satellite(GPSConstellation::GLONASS, 7, 45, 180, 40, Use::Used),
            satellite(GPSConstellation::Galileo, 11, 30, 270, 45, Use::Unknown),
            satellite(GPSConstellation::BeiDou, 36, 60, 135, 38, Use::Used),
            satellite(GPSConstellation::SBAS, 136, 35, 194, 35, Use::Unused)};
}

/// As many satellites as a report lists, of every constellation in turn.
std::vector<GPSSatellite> fullSky()
{
    std::vector<GPSSatellite> satellites;
    for (int constellation = 0; constellation < 8; ++constellation) {
        for (int id = 1; id <= int(GPSSatelliteReport::MAX_SATELLITES) / 8; ++id) {
            satellites.push_back(satellite(static_cast<GPSConstellation>(constellation), 140 + id, id * 5, id * 22,
                                           20 + id * 2, id % 3 ? Use::Used : Use::Unused));
        }
    }
    return satellites;
}

/// The width of a character of the default font, which the panels are sized in.
qreal characterWidth(TestFixtures::QmlEngine& engine)
{
    const auto metrics =
        engine.create(QByteArray("import QtQml\nimport QGroundControl.Controls\n"
                                 "QtObject { property real character: "
                                 "ScreenTools.defaultFontPixelWidth }"));
    return metrics ? metrics->property("character").toReal() : 0;
}

/// The receiver status of @a receiver, @a width wide, in @a window; null on failure, with the reason in the engine's
/// lastError().
std::unique_ptr<QObject> createStatus(TestFixtures::QmlEngine& engine, const TestFixtures::QmlWindowFixture& window,
                                      GPSReceiver& receiver, qreal width)
{
    return engine.create(TestFixtures::sourceQmlUrl(QStringLiteral("GPS/Qml/GPSReceiverStatus.qml")),
                         {{QStringLiteral("parent"), QVariant::fromValue(window.contentItem())},
                          {QStringLiteral("receiver"), QVariant::fromValue(&receiver)},
                          {QStringLiteral("facts"), QVariant::fromValue(receiver.facts())},
                          {QStringLiteral("width"), width}});
}

QList<QQuickItem*> shownItems(QQuickItem* root, const char* name)
{
    return TestFixtures::findItems(root, [objectName = QString::fromLatin1(name)](QQuickItem* item) {
        return item->objectName() == objectName && item->isVisible();
    });
}

/// The items shown under @a view that reach outside @a box, each with its extent; empty when all are inside it.
QStringList itemsOutside(QQuickItem* view, QQuickItem* box)
{
    QStringList outside;
    const auto shown = TestFixtures::findItems(
        view, [](QQuickItem* item) { return item->isVisible() && item->width() > 0 && item->height() > 0; });
    for (QQuickItem* const item : shown) {
        const QRectF rect = item->mapRectToItem(box, item->boundingRect());
        if (rect.left() < -1 || rect.right() > box->width() + 1 || rect.top() < -1 ||
            rect.bottom() > box->height() + 1) {
            outside.append(
                QStringLiteral("%1 \"%2\" at %3,%4 to %5,%6 in a box of %7 by %8")
                    .arg(QString::fromLatin1(item->metaObject()->className()), item->property("text").toString())
                    .arg(rect.left())
                    .arg(rect.top())
                    .arg(rect.right())
                    .arg(rect.bottom())
                    .arg(box->width())
                    .arg(box->height()));
        }
    }
    return outside;
}

/// The labels shown under @a view whose text is wider than the label.
QStringList textsCut(QQuickItem* view)
{
    QStringList cut;
    const auto texts = TestFixtures::findItems(view, [](QQuickItem* item) {
        return item->isVisible() && item->property("contentWidth").isValid() &&
               !item->property("text").toString().isEmpty();
    });
    for (QQuickItem* const text : texts) {
        if (text->property("contentWidth").toReal() > text->width() + 1) {
            cut.append(QStringLiteral("\"%1\" is %2 wide in %3")
                           .arg(text->property("text").toString())
                           .arg(text->property("contentWidth").toReal())
                           .arg(text->width()));
        }
    }
    return cut;
}

QPointF centerIn(QQuickItem* item, QQuickItem* box)
{
    return item->mapToItem(box, QPointF(item->width() / 2, item->height() / 2));
}

}  // namespace

void GPSSatelliteViewTest::_receiverListsItsSatellites()
{
    ManualScheduler scheduler;
    ScriptedGPSReceiver harness(&scheduler);
    GPSReceiver& receiver = harness.receiver;
    GPSSatelliteModel* const model = receiver.satellites();
    QVERIFY(model);
    QCOMPARE(receiver.property("satellites").value<GPSSatelliteModel*>(), model);
    QCOMPARE(model->count(), 0);
    QVERIFY(connectOverTcp(receiver));
    ScriptedReceiverWorker* worker = harness.workers.current();
    worker->ready();
    const std::vector<GPSSatellite> sky = knownSky();
    worker->satellites(reportOf(sky));
    QCOMPARE(model->count(), int(sky.size()));
    const QModelIndex sbas = model->index(model->count() - 1);
    QCOMPARE(sbas.data(GPSSatelliteModel::ConstellationRole).toInt(), int(GPSSatelliteModel::Constellation::SBAS));
    QCOMPARE(sbas.data(GPSSatelliteModel::ConstellationNameRole).toString(), GPSReceiver::tr("SBAS"));
    QCOMPARE(sbas.data(GPSSatelliteModel::SatelliteIdRole).toInt(), 136);
    QCOMPARE(sbas.data(GPSSatelliteModel::ElevationRole).toInt(), 35);
    QCOMPARE(sbas.data(GPSSatelliteModel::AzimuthRole).toInt(), 194);
    QCOMPARE(sbas.data(GPSSatelliteModel::SignalStrengthRole).toInt(), 35);
    QCOMPARE(sbas.data(GPSSatelliteModel::UsedRole).toBool(), false);
    QCOMPARE(model->index(0).data(GPSSatelliteModel::UsedRole).toBool(), true);

    // A report that lists none, as when the receiver's satellite reports ran out, empties the model.
    worker->satellites(GPSSatelliteReport{});
    QCOMPARE(model->count(), 0);

    // So does a receiver that stays connected and falls silent, as its other status does.
    worker->satellites(reportOf(sky));
    worker->position(fixReport(GPSFixQuality::Fix3D));
    QCOMPARE(model->count(), int(sky.size()));
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(6)));
    QCOMPARE(model->count(), 0);

    // And the end of the connection.
    worker->satellites(reportOf(sky));
    QCOMPARE(model->count(), int(sky.size()));
    receiver.disconnectReceiver();
    QCOMPARE(model->count(), 0);

    // An observer of the status may end the connection as a report arrives; its satellites do not stay behind.
    QVERIFY(connectOverTcp(receiver));
    worker = harness.workers.current();
    worker->ready();
    const QMetaObject::Connection observer = connect(receiver.facts()->numSatellites(), &Fact::rawValueChanged,
                                                     &receiver, [&receiver]() { receiver.disconnectReceiver(); });
    worker->satellites(reportOf(sky));
    disconnect(observer);
    QCOMPARE(receiver.state(), GPSReceiver::State::Disconnected);
    QCOMPARE(model->count(), 0);
}

void GPSSatelliteViewTest::_viewStaysInsideItsBox_data()
{
    QTest::addColumn<int>("characters");
    QTest::addColumn<bool>("startsShown");
    QTest::addColumn<bool>("sideBySide");
    // The status column of the toolbar's drop-down.
    QTest::newRow("drop-down") << 36 << false << false;
    // The settings page at its narrowest, and on a wide window.
    QTest::newRow("settings-page") << 50 << true << false;
    QTest::newRow("wide-settings-page") << 90 << true << true;
}

void GPSSatelliteViewTest::_viewStaysInsideItsBox()
{
    QFETCH(int, characters);
    QFETCH(bool, startsShown);
    QFETCH(bool, sideBySide);
    ScriptedGPSReceiver harness;
    GPSReceiver& receiver = harness.receiver;
    TestFixtures::QmlWindowFixture window(1200, 1600);
    TestFixtures::QmlEngine engine;
    const qreal character = characterWidth(engine);
    QVERIFY2(character > 0, qPrintable(engine.lastError()));
    const std::unique_ptr<QObject> status = createStatus(engine, window, receiver, characters * character);
    QVERIFY2(status, qPrintable(engine.lastError()));
    auto* const box = qobject_cast<QQuickItem*>(status.get());
    auto* const view = status->findChild<QQuickItem*>(QStringLiteral("rtkSatelliteView"));
    QVERIFY(box && view);
    QVERIFY(window.show());
    QVERIFY(connectOverTcp(receiver));
    ScriptedReceiverWorker* const worker = harness.workers.current();
    worker->ready();
    // A receiver that describes no satellite has no view of them.
    QVERIFY(!view->isVisible());

    worker->satellites(reportOf(fullSky()));
    QVERIFY(QQuickTest::qWaitForPolish(window.window(), TestTimeout::mediumMs()));
    QVERIFY(view->isVisible());
    QCOMPARE(box->width(), characters * character);
    // In a panel as narrow as the drop-down's, neither is created until it is asked for.
    QCOMPARE(view->property("expanded").toBool(), startsShown);
    QCOMPARE(view->findChild<QQuickItem*>(QStringLiteral("gpsSkyPlot")) != nullptr, startsShown);
    QCOMPARE(view->findChild<QQuickItem*>(QStringLiteral("gpsSatelliteSignalBars")) != nullptr, startsShown);
    auto* const toggleRow = view->findChild<QQuickItem*>(QStringLiteral("gpsSatelliteViewToggle"));
    QVERIFY(toggleRow);
    const auto buttons =
        TestFixtures::findItems(toggleRow, [](QQuickItem* item) { return item->inherits("QQuickAbstractButton"); });
    QCOMPARE(buttons.size(), 1);
    QQuickItem* const toggle = buttons.first();
    if (!startsShown) {
        const qreal collapsedHeight = box->implicitHeight();
        QCOMPARE(toggle->property("text").toString(), QStringLiteral("Show"));
        QVERIFY(QMetaObject::invokeMethod(toggle, "click"));
        QVERIFY(view->property("expanded").toBool());
        QVERIFY(QQuickTest::qWaitForPolish(window.window(), TestTimeout::mediumMs()));
        QVERIFY(box->implicitHeight() > collapsedHeight);
        // What was asked for stays, whatever the panel's width becomes.
        box->setWidth(box->width() - character);
        QVERIFY(QQuickTest::qWaitForPolish(window.window(), TestTimeout::mediumMs()));
        QVERIFY(view->property("expanded").toBool());
        box->setWidth(characters * character);
        QVERIFY(QQuickTest::qWaitForPolish(window.window(), TestTimeout::mediumMs()));
    }
    QCOMPARE(toggle->property("text").toString(), QStringLiteral("Hide"));
    auto* const plot = view->findChild<QQuickItem*>(QStringLiteral("gpsSkyPlot"));
    auto* const bars = view->findChild<QQuickItem*>(QStringLiteral("gpsSatelliteSignalBars"));
    QVERIFY(plot && bars);

    // Every satellite has its bar and its place in the sky, and all of it is inside the status box, however many
    // satellites there are.
    const int count = receiver.satellites()->count();
    QCOMPARE(count, int(GPSSatelliteReport::MAX_SATELLITES));
    QCOMPARE(shownItems(bars, "gpsSatelliteBar").size(), count);
    QCOMPARE(shownItems(plot, "gpsSkyPlotSatellite").size(), count);
    QCOMPARE(shownItems(bars, "gpsSatelliteBarConstellation").size(), 8);
    QCOMPARE(box->width(), characters * character);
    QStringList outside = itemsOutside(view, box);
    QVERIFY2(outside.isEmpty(), qPrintable(outside.join(QStringLiteral("; "))));
    // The plot is a square, and the satellites are inside it.
    QCOMPARE(plot->width(), plot->height());
    outside = itemsOutside(plot, plot);
    QVERIFY2(outside.isEmpty(), qPrintable(outside.join(QStringLiteral("; "))));
    // Three-digit numbers fit their bars and markers.
    const QStringList cut = textsCut(view);
    QVERIFY2(cut.isEmpty(), qPrintable(cut.join(QStringLiteral("; "))));
    // The bars wrap below one another, beside the plot where the panel is wide.
    QVERIFY(bars->height() > 2 * shownItems(bars, "gpsSatelliteBar").first()->height());
    const QRectF plotRect = plot->mapRectToItem(view, plot->boundingRect());
    const QRectF barsRect = bars->mapRectToItem(view, bars->boundingRect());
    QVERIFY(sideBySide ? barsRect.left() >= plotRect.right() : barsRect.top() >= plotRect.bottom());

    // Hidden again, they are gone and the panel is as short as without them.
    const qreal expandedHeight = box->implicitHeight();
    const qreal plotHeight = plot->height();
    QVERIFY(QMetaObject::invokeMethod(toggle, "click"));
    QVERIFY(!view->property("expanded").toBool());
    QCOMPARE(toggle->property("text").toString(), QStringLiteral("Show"));
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(!view->findChild<QQuickItem*>(QStringLiteral("gpsSkyPlot")));
    QVERIFY(!view->findChild<QQuickItem*>(QStringLiteral("gpsSatelliteSignalBars")));
    QVERIFY(QQuickTest::qWaitForPolish(window.window(), TestTimeout::mediumMs()));
    QVERIFY(box->implicitHeight() < expandedHeight - plotHeight);

    receiver.disconnectReceiver();
    QVERIFY(!view->isVisible());
}

void GPSSatelliteViewTest::_viewMarksSatellites()
{
    ScriptedGPSReceiver harness;
    GPSReceiver& receiver = harness.receiver;
    TestFixtures::QmlWindowFixture window(1200, 1200);
    TestFixtures::QmlEngine engine;
    const qreal character = characterWidth(engine);
    QVERIFY2(character > 0, qPrintable(engine.lastError()));
    const std::unique_ptr<QObject> status = createStatus(engine, window, receiver, 90 * character);
    QVERIFY2(status, qPrintable(engine.lastError()));
    QVERIFY(window.show());
    QVERIFY(connectOverTcp(receiver));
    ScriptedReceiverWorker* const worker = harness.workers.current();
    worker->ready();
    std::vector<GPSSatellite> sky = knownSky();
    worker->satellites(reportOf(sky));
    QVERIFY(QQuickTest::qWaitForPolish(window.window(), TestTimeout::mediumMs()));
    auto* const plot = status->findChild<QQuickItem*>(QStringLiteral("gpsSkyPlot"));
    auto* const bars = status->findChild<QQuickItem*>(QStringLiteral("gpsSatelliteSignalBars"));
    QVERIFY(plot && bars);
    const QGCPalette palette;
    const auto colorOf = [](const QQuickItem* item) { return item->property("color").value<QColor>(); };
    const auto numbered = [](const QList<QQuickItem*>& items, int id) -> QQuickItem* {
        for (QQuickItem* const item : items) {
            if (item->property("satelliteId").toInt() == id) {
                return item;
            }
        }
        return nullptr;
    };

    // The sky from above, north up: overhead is the centre, the horizon the rim, and east is to the right. The
    // satellite the receiver has no direction of is not in it.
    const auto markers = shownItems(plot, "gpsSkyPlotSatellite");
    QCOMPARE(markers.size(), int(sky.size()) - 1);
    QVERIFY(!numbered(markers, 12));
    QQuickItem* const overhead = numbered(markers, 2);
    QQuickItem* const north = numbered(markers, 5);
    QQuickItem* const east = numbered(markers, 25);
    QQuickItem* const south = numbered(markers, 7);
    QQuickItem* const west = numbered(markers, 11);
    QVERIFY(overhead && north && east && south && west);
    const QPointF center(plot->width() / 2, plot->height() / 2);
    const qreal radius = plot->property("_radius").toReal();
    QVERIFY(radius > 0);
    const auto at = [plot](QQuickItem* marker, const QPointF& expected) {
        const QPointF offset = centerIn(marker, plot) - expected;
        return qAbs(offset.x()) <= 1 && qAbs(offset.y()) <= 1;
    };
    QVERIFY(at(overhead, center));
    QVERIFY(at(north, center + QPointF(0, -radius)));
    QVERIFY(at(east, center + QPointF(radius, 0)));
    // At 45 degrees of elevation to the south and at 30 to the west.
    QVERIFY(at(south, center + QPointF(0, radius / 2)));
    QVERIFY(at(west, center + QPointF(-radius * 2 / 3, 0)));

    // A satellite has its constellation's colour, in the plot and in the bars, and is filled when the receiver uses
    // it. One the receiver does not use, or does not say that of, is an outline.
    const auto levels = shownItems(bars, "gpsSatelliteBarLevel");
    const auto barItems = shownItems(bars, "gpsSatelliteBar");
    QCOMPARE(barItems.size(), int(sky.size()));
    QCOMPARE(levels.size(), int(sky.size()));
    const auto levelOf = [&](int id) { return levels.at(barItems.indexOf(numbered(barItems, id))); };
    QCOMPARE(colorOf(overhead), palette.colorGreen());
    QCOMPARE(colorOf(levelOf(2)), palette.colorGreen());
    QCOMPARE(colorOf(south), palette.colorRed());
    QCOMPARE(colorOf(levelOf(7)), palette.colorRed());
    for (QQuickItem* const outline : {east, west, levelOf(25), levelOf(11)}) {
        QCOMPARE(colorOf(outline).alpha(), 0);
    }
    QCOMPARE(east->property("border").value<QObject*>()->property("color").value<QColor>(), palette.colorGreen());
    QCOMPARE(west->property("border").value<QObject*>()->property("color").value<QColor>(), palette.colorBlue());

    // A bar is as tall as the signal is strong, full at 50 dB-Hz; a satellite without a signal keeps a line.
    const qreal full = levelOf(5)->height();
    QCOMPARE(levelOf(5)->parentItem()->height(), full);
    QVERIFY(qAbs(levelOf(25)->height() - full / 2) <= 1);
    QCOMPARE(levelOf(12)->height(), 1.0);
    // Each constellation is named once, before its first bar.
    QStringList named;
    for (QQuickItem* const name : shownItems(bars, "gpsSatelliteBarConstellation")) {
        named.append(name->property("text").toString());
    }
    QCOMPARE(named, QStringList({"GPS", "GLONASS", "Galileo", "BeiDou", "SBAS"}));

    // The next epoch moves and recolours what changed, without a new set of items.
    sky[0].use = Use::Unused;
    sky[3] = satellite(GPSConstellation::GPS, 25, 90, 0, 50, Use::Used);
    worker->satellites(reportOf(sky));
    QVERIFY(QQuickTest::qWaitForPolish(window.window(), TestTimeout::mediumMs()));
    QCOMPARE(shownItems(plot, "gpsSkyPlotSatellite"), markers);
    QCOMPARE(colorOf(overhead).alpha(), 0);
    QVERIFY(at(east, center));
    QCOMPARE(colorOf(east), palette.colorGreen());
    QCOMPARE(levelOf(25)->height(), full);
}

UT_REGISTER_TEST(GPSSatelliteViewTest, TestLabel::Unit)
