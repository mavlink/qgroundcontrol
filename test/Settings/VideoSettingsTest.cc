#include "VideoSettingsTest.h"

#include <QtCore/QSettings>
#include <QtTest/QTest>

#include "VideoSettings.h"

void VideoSettingsTest::_streamEnabledMigration_data()
{
    QTest::addColumn<bool>("storedStreamEnabled");
    QTest::addColumn<bool>("disableWhenDisarmed");
    QTest::addColumn<bool>("expectStored");

    QTest::addRow("disabled while disarmed") << false << true << false;
    QTest::addRow("disabled, disable when disarmed turned off") << false << false << false;
    QTest::addRow("enabled") << true << true << true;
}

void VideoSettingsTest::_streamEnabledMigration()
{
    QFETCH(bool, storedStreamEnabled);
    QFETCH(bool, disableWhenDisarmed);
    QFETCH(bool, expectStored);

    QSettings settings;
    settings.beginGroup(VideoSettings::settingsGroup);
    settings.setValue(VideoSettings::streamEnabledName, storedStreamEnabled);
    settings.setValue(VideoSettings::disableWhenDisarmedName, disableWhenDisarmed);
    settings.endGroup();

    // Migration runs in the constructor. SettingsFacts ignore QSettings under unit tests, so check the stored value.
    const VideoSettings videoSettings;
    settings.beginGroup(VideoSettings::settingsGroup);
    QCOMPARE(settings.contains(VideoSettings::streamEnabledName), expectStored);
    QVERIFY(settings.contains(VideoSettings::disableWhenDisarmedName));
    settings.endGroup();
}

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

UT_REGISTER_TEST(VideoSettingsTest, TestLabel::Unit)
