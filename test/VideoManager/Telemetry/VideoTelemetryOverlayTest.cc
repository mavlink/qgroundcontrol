#include "VideoTelemetryOverlayTest.h"

#include <QtTest/QSignalSpy>

#include "FactValueGrid.h"
#include "HorizontalFactValueGrid.h"
#include "InstrumentValueData.h"
#include "MultiVehicleManager.h"
#include "QmlObjectListModel.h"
#include "SettingsManager.h"
#include "Vehicle.h"
#include "VideoSettings.h"
#include "VideoTelemetryOverlay.h"
#include "VideoTestHelpers.h"

void VideoTelemetryOverlayTest::_testFollowsSettingAndVehicle()
{
    Fact* const enabled = SettingsManager::instance()->videoSettings()->showTelemetryOverlay();
    VideoTestHelpers::FactRestorer restorer;
    restorer.set(enabled, false);

    VideoTelemetryOverlay overlay(enabled);
    QVERIFY(overlay.text().isEmpty());
    QVERIFY(!overlay._timer.isActive());

    QSignalSpy textChanged(&overlay, &VideoTelemetryOverlay::textChanged);
    QVERIFY(textChanged.isValid());
    restorer.set(enabled, true);
    QVERIFY(textChanged.wait(TestTimeout::mediumMs()));
    // Values are apart by plain spaces, which the overlay may wrap at; each value itself holds only non-breaking ones.
    const QStringList values = overlay.text().split(QStringLiteral("    "));
    QVERIFY(!values.isEmpty());
    for (const QString& value : values) {
        QVERIFY2(value.contains(QStringLiteral(":") + QChar(QChar::Nbsp)), qPrintable(value));
        QVERIFY2(!value.contains(u' '), qPrintable(value));
    }

    // The telemetry Facts belong to the vehicle: once it is gone, the overlay is empty rather than reading freed Facts.
    _disconnectMockLink();
    QVERIFY(!MultiVehicleManager::instance()->activeVehicle());
    QTRY_VERIFY_WITH_TIMEOUT(overlay.text().isEmpty(), TestTimeout::mediumMs());

    _connectMockLink();
    QTRY_VERIFY_WITH_TIMEOUT(!overlay.text().isEmpty(), TestTimeout::mediumMs());
    textChanged.clear();
    restorer.set(enabled, false);
    QCOMPARE(textChanged.count(), 1);
    QVERIFY(overlay.text().isEmpty());
    QVERIFY(!overlay._timer.isActive());
}

void VideoTelemetryOverlayTest::_testFollowsTelemetryBarEdits()
{
    Fact* const enabled = SettingsManager::instance()->videoSettings()->showTelemetryOverlay();
    VideoTestHelpers::FactRestorer restorer;
    restorer.set(enabled, true);
    VideoTelemetryOverlay overlay(enabled);

    Vehicle* const vehicle = MultiVehicleManager::instance()->activeVehicle();
    QVERIFY(vehicle);
    const QString heading = vehicle->getFact(QStringLiteral("heading"))->shortDescription() + QStringLiteral(":");
    QTRY_VERIFY_WITH_TIMEOUT(!overlay.text().isEmpty(), TestTimeout::mediumMs());
    QVERIFY(!overlay.text().contains(heading));

    // The user puts the heading in the bar's first cell.
    FactValueGrid bar;
    (void) bar.setProperty("settingsGroup", HorizontalFactValueGrid::telemetryBarSettingsGroup);
    bar.componentComplete();
    const QmlObjectListModel* const column = bar.columns()->value<const QmlObjectListModel*>(0);
    column->value<InstrumentValueData*>(0)->setFact(QStringLiteral("Vehicle"), QStringLiteral("Heading"));
    QTRY_VERIFY_WITH_TIMEOUT(overlay.text().contains(heading), TestTimeout::mediumMs());
}

UT_REGISTER_TEST(VideoTelemetryOverlayTest, TestLabel::Integration, TestLabel::Vehicle)
