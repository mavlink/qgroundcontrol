#include "VideoSettingsTest.h"

#include <memory>

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "ColoredSvgImageProvider.h"
#include "VideoManager/VideoTestHelpers.h"
#include "VideoSettings.h"

namespace {

const QString kTestCamera = QStringLiteral("QGC Test Camera");
const QString kOtherTestCamera = QStringLiteral("QGC Other Test Camera");

// The control the settings page shows for videoSource.
constexpr const char* kVideoSourceComboBoxQml = R"QML(
import QGroundControl.FactControls

FactComboBox {
}
)QML";

}  // namespace

void VideoSettingsTest::_videoFitEnumMatchesMetaData()
{
    VideoSettings videoSettings;
    const Fact* const videoFit = videoSettings.videoFit();
    QCOMPARE(videoFit->enumStrings(), QStringList({QStringLiteral("Fit Width"), QStringLiteral("Fit Height"),
                                                   QStringLiteral("Fill"), QStringLiteral("No Crop")}));
    const QVariantList values = videoFit->enumValues();
    QCOMPARE(values.size(), 4);
    QCOMPARE(values.at(0).toInt(), static_cast<int>(VideoSettings::VideoFitWidth));
    QCOMPARE(values.at(1).toInt(), static_cast<int>(VideoSettings::VideoFitHeight));
    QCOMPARE(values.at(2).toInt(), static_cast<int>(VideoSettings::VideoFitFill));
    QCOMPARE(values.at(3).toInt(), static_cast<int>(VideoSettings::VideoFitNoCrop));
}

void VideoSettingsTest::_videoSourcesFollowCameras()
{
    // The test's cameras stand in for this computer's. _updateVideoSources() is what a camera plugged in or unplugged
    // runs.
    QStringList cameras;
    VideoSettings videoSettings;
    videoSettings._cameraNames = [&cameras] { return cameras; };
    videoSettings._updateVideoSources();
    Fact* const videoSource = videoSettings.videoSource();
    const QVariantList fixedSources = videoSource->enumValues();
    QCOMPARE(fixedSources.first().toString(), QString::fromLatin1(VideoSettings::videoDisabled));
    QVERIFY(fixedSources.contains(QString::fromLatin1(VideoSettings::videoSourceRTSP)));

    VideoTestHelpers::InlineQmlEngine engine;
    // For the combo box's arrow.
    engine.addImageProvider(QLatin1String(ColoredSvgImageProvider::ProviderId), new ColoredSvgImageProvider());
    const std::unique_ptr<QObject> comboBox =
        engine.create(kVideoSourceComboBoxQml, {{QStringLiteral("fact"), QVariant::fromValue(videoSource)}});
    QVERIFY2(comboBox, qPrintable(engine.error()));

    // Cameras plugged in are offered after the fixed sources, in the settings page's combo box too.
    cameras = {kTestCamera, kOtherTestCamera};
    videoSettings._updateVideoSources();
    QCOMPARE(videoSource->enumValues(), fixedSources + QVariantList({kTestCamera, kOtherTestCamera}));
    QCOMPARE(comboBox->property("count").toInt(), fixedSources.size() + 2);
    videoSource->setRawValue(kOtherTestCamera);
    QTRY_COMPARE_WITH_TIMEOUT(comboBox->property("currentText").toString(), kOtherTestCamera, TestTimeout::shortMs());

    // An unplugged camera that isn't selected is no longer offered, and the selected one still is.
    cameras = {kOtherTestCamera};
    videoSettings._updateVideoSources();
    QCOMPARE(videoSource->enumValues(), fixedSources + QVariantList({kOtherTestCamera}));
    QCOMPARE(comboBox->property("count").toInt(), fixedSources.size() + 1);
    QTRY_COMPARE_WITH_TIMEOUT(comboBox->property("currentText").toString(), kOtherTestCamera, TestTimeout::shortMs());

    // With the cameras as they were, the combo box is left alone.
    QSignalSpy enumsSpy(videoSource, &Fact::enumsChanged);
    videoSettings._updateVideoSources();
    QCOMPARE(enumsSpy.count(), 0);
}

void VideoSettingsTest::_absentStoredCameraStaysSelected_data()
{
    QTest::addColumn<QString>("setting");
    QTest::addColumn<QString>("noCamera");  ///< The setting's first choice, which selects no camera

    QTest::newRow("video source") << QString::fromLatin1(VideoSettings::videoSourceName)
                                  << QString::fromLatin1(VideoSettings::videoDisabled);
    QTest::newRow("tile camera") << QString::fromLatin1(VideoSettings::uvcTileCameraName) << QString();
}

void VideoSettingsTest::_absentStoredCameraStaysSelected()
{
    QFETCH(QString, setting);
    QFETCH(QString, noCamera);

    const QString notConnected = QStringLiteral("QGC Test Camera (not connected)");

    QStringList cameras;
    VideoSettings videoSettings;
    videoSettings._cameraNames = [&cameras] { return cameras; };
    // SettingsFacts ignore QSettings under unit tests, so the default stands in for the stored camera.
    videoSettings._nameToMetaDataMap[setting]->setRawDefaultValue(kTestCamera);
    Fact* const fact = videoSettings.property(setting.toLatin1().constData()).value<Fact*>();
    QVERIFY(fact);

    // A stored camera that isn't attached at startup stays selected, and listed as such.
    QCOMPARE(fact->rawValue().toString(), kTestCamera);
    QCOMPARE(fact->enumValues().first().toString(), noCamera);
    QCOMPARE(fact->enumValues().last().toString(), kTestCamera);
    QCOMPARE(fact->enumStringValue(), notConnected);

    // Plugged in, it is one of the cameras.
    cameras = {kOtherTestCamera, kTestCamera};
    videoSettings._updateVideoSources();
    QCOMPARE(fact->enumValues().mid(fact->enumValues().size() - 2), QVariantList({kOtherTestCamera, kTestCamera}));
    QCOMPARE(fact->enumStringValue(), kTestCamera);

    // Unplugged while selected, it stays selected.
    cameras.clear();
    videoSettings._updateVideoSources();
    QCOMPARE(fact->rawValue().toString(), kTestCamera);
    QCOMPARE(fact->enumStringValue(), notConnected);

    // Once no camera is selected, it is no longer offered.
    fact->setRawValue(noCamera);
    QTRY_VERIFY_WITH_TIMEOUT(!fact->enumValues().contains(kTestCamera), TestTimeout::shortMs());
    QCOMPARE(fact->rawValue().toString(), noCamera);
}

void VideoSettingsTest::_overriddenVideoSourcesAreKept()
{
    const QVariantList sources(
        {QString::fromLatin1(VideoSettings::videoSourceRTSP), QString::fromLatin1(VideoSettings::videoDisabled)});

    VideoSettings videoSettings;
    Fact* const videoSource = videoSettings.videoSource();
    // As a settings override file's enumValues leave the sources when the Fact is created.
    videoSource->setEnumInfo({QStringLiteral("RTSP"), QStringLiteral("Off")}, sources);

    videoSettings._updateVideoSources();
    QCOMPARE(videoSource->enumValues(), sources);
}

UT_REGISTER_TEST(VideoSettingsTest, TestLabel::Unit)
