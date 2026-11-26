#include "VideoManagerTestBase.h"

#include <QtCore/QCoreApplication>

#include "SettingsManager.h"
#include "VideoManager.h"
#include "VideoSettings.h"
#include "VideoStreamController.h"
#include "VideoTestHelpers.h"

VideoManagerTestBase::~VideoManagerTestBase() = default;

void VideoManagerTestBase::init()
{
    UnitTest::init();

    _videoSettings = SettingsManager::instance()->videoSettings();
    _facts = std::make_unique<VideoTestHelpers::FactRestorer>();
    _facts->set(_videoSettings->videoSource(), QString(VideoSettings::videoSourceUDPH264));
    _facts->set(_videoSettings->udpUrl(), QStringLiteral("0.0.0.0:5600"));
    _facts->set(_videoSettings->rtspUrl(), QStringLiteral("rtsp://cam.local/manual"));
    _facts->set(_videoSettings->disableWhenDisarmed(), false);
    _facts->set(_videoSettings->lowLatencyMode(), false);
    _facts->set(_videoSettings->rtpJitterLatencyMs(), 80);
    _facts->set(_videoSettings->rtspAutoReconnect(), true);
    _facts->set(_videoSettings->rtspTimeout(), 8);
    _facts->set(_videoSettings->uvcTileCameraAsMainPicture(), false);

    _videoManager = std::make_unique<VideoManager>();
    _videoManager->_receiverFactory = [](QObject* parent) { return new FakeVideoReceiver(parent); };
    _videoManager->_createVideoReceivers();
    // A row of tiles with room for them all is shown, as when the video is the Fly View's main view.
    _videoManager->setTileRoom(4);
    _primary = qobject_cast<FakeVideoReceiver*>(_videoManager->primaryStream()->receiver());
    _thermal = qobject_cast<FakeVideoReceiver*>(_videoManager->thermalStream()->receiver());
    QVERIFY(_primary);
    QVERIFY(_thermal);
    _settle();
    _resetCounters();
}

void VideoManagerTestBase::cleanup()
{
    _primary = nullptr;
    _thermal = nullptr;
    _videoManager.reset();
    _facts.reset();
    _settle();

    UnitTest::cleanup();
}

void VideoManagerTestBase::_settle()
{
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
}

void VideoManagerTestBase::_resetCounters()
{
    for (const VideoStreamController* source : std::as_const(_videoManager->_sources)) {
        if (FakeVideoReceiver* const receiver = _receiverOf(source)) {
            receiver->configChanges = 0;
            receiver->activeChanges = 0;
            receiver->restarts = 0;
            receiver->videoOutputChanges = 0;
        }
    }
}

FakeVideoReceiver* VideoManagerTestBase::_receiverOf(const VideoStreamController* source)
{
    return qobject_cast<FakeVideoReceiver*>(source->receiver());
}
