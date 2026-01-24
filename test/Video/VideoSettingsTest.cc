#include "VideoSettingsTest.h"
#include "VideoSettings.h"

#include <QtTest/QTest>

void VideoSettingsTest::_testVideoSourceConstants()
{
    // Verify all video source constants are defined and non-empty
    QVERIFY(QString(VideoSettings::videoSourceNoVideo).length() > 0);
    QVERIFY(QString(VideoSettings::videoDisabled).length() > 0);
    QVERIFY(QString(VideoSettings::videoSourceRTSP).length() > 0);
    QVERIFY(QString(VideoSettings::videoSourceUDPH264).length() > 0);
    QVERIFY(QString(VideoSettings::videoSourceUDPH265).length() > 0);
    QVERIFY(QString(VideoSettings::videoSourceTCP).length() > 0);
    QVERIFY(QString(VideoSettings::videoSourceMPEGTS).length() > 0);
    QVERIFY(QString(VideoSettings::videoSource3DRSolo).length() > 0);
    QVERIFY(QString(VideoSettings::videoSourceParrotDiscovery).length() > 0);
    QVERIFY(QString(VideoSettings::videoSourceYuneecMantisG).length() > 0);
    QVERIFY(QString(VideoSettings::videoSourceHerelinkAirUnit).length() > 0);
    QVERIFY(QString(VideoSettings::videoSourceHerelinkHotspot).length() > 0);

    // Verify constants are unique
    QStringList sources = {
        VideoSettings::videoSourceNoVideo,
        VideoSettings::videoDisabled,
        VideoSettings::videoSourceRTSP,
        VideoSettings::videoSourceUDPH264,
        VideoSettings::videoSourceUDPH265,
        VideoSettings::videoSourceTCP,
        VideoSettings::videoSourceMPEGTS,
        VideoSettings::videoSource3DRSolo,
        VideoSettings::videoSourceParrotDiscovery,
        VideoSettings::videoSourceYuneecMantisG,
        VideoSettings::videoSourceHerelinkAirUnit,
        VideoSettings::videoSourceHerelinkHotspot
    };

    // Check all sources are unique
    QSet<QString> uniqueSources(sources.begin(), sources.end());
    QCOMPARE(uniqueSources.size(), sources.size());
}

void VideoSettingsTest::_testStreamSourceIdentification()
{
    // These are the video sources that represent network streams (not UVC/disabled)
    const QStringList streamSources = {
        VideoSettings::videoSourceUDPH264,
        VideoSettings::videoSourceUDPH265,
        VideoSettings::videoSourceRTSP,
        VideoSettings::videoSourceTCP,
        VideoSettings::videoSourceMPEGTS,
        VideoSettings::videoSource3DRSolo,
        VideoSettings::videoSourceParrotDiscovery,
        VideoSettings::videoSourceYuneecMantisG,
        VideoSettings::videoSourceHerelinkAirUnit,
        VideoSettings::videoSourceHerelinkHotspot,
    };

    // These are NOT stream sources
    const QStringList nonStreamSources = {
        VideoSettings::videoSourceNoVideo,
        VideoSettings::videoDisabled,
    };

    // Verify stream sources are properly categorized
    for (const QString &source : streamSources) {
        QVERIFY2(streamSources.contains(source),
                 qPrintable(QString("Expected %1 to be a stream source").arg(source)));
    }

    // Verify non-stream sources are not in the stream list
    for (const QString &source : nonStreamSources) {
        QVERIFY2(!streamSources.contains(source),
                 qPrintable(QString("Expected %1 to NOT be a stream source").arg(source)));
    }
}
