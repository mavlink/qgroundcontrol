#pragma once

#include <chrono>
#include <memory>

#include <QtCore/QList>
#include <QtCore/QRegularExpression>
#include <QtCore/QString>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickItem>
#include <QtTest/QSignalSpy>

#include "QtMultimediaVideoReceiver.h"
#include "QtMultimediaVideoReceiverTest.h"
#include "UnitTest.h"
#include "VideoStreamStats.h"
#include "VideoTestHelpers.h"

/// What the tests that run a QtMultimediaVideoReceiver share, and the log lines every test of a QtVideoRecorder lets
/// pass.
namespace QtMultimediaTest {

/// A log line a test lets pass: the arguments of UnitTest::ignoreLogMessage(), which only a test class can call.
struct IgnoredLog
{
    const char* category;
    QtMsgType type;
    QRegularExpression pattern;
};

/// Qt's FFmpeg player warns that a live stream can't loop, though nothing asked it to, and tells when the machine has
/// no hardware decoder for the stream.
inline QList<IgnoredLog> playerLogs()
{
    return {
        {"default", QtWarningMsg, QRegularExpression(QStringLiteral("^Cannot set loops for non-seekable source$"))},
        {"qt.multimedia.playbackengine.codec", QtInfoMsg, QRegularExpression(QStringLiteral("No HW decoder found"))},
    };
}

/// Qt reports each format this platform can't encode, which the recorder moves past.
inline QList<IgnoredLog> encoderFallbackLogs()
{
    return {
        {"qt.multimedia.ffmpeg.encoder", QtInfoMsg, QRegularExpression(QStringLiteral("^Recording new media"))},
        {"default", QtWarningMsg,
         QRegularExpression(QStringLiteral("^(Cannot find software target format|Could not find target format)"))},
        {"qt.multimedia.ffmpeg.videoencoder", QtWarningMsg,
         QRegularExpression(QStringLiteral("^No valid video codecs found"))},
        {"Video.QtVideoRecorder", QtWarningMsg,
         QRegularExpression(QStringLiteral("^Recording error: .* \"Could not initialize encoder\"$"))},
    };
}

/// A QtMultimediaVideoReceiver playing @p uri into a VideoOutput, with spies on its signals.
struct ReceiverUnderTest
{
    explicit ReceiverUnderTest(const QString& uri, bool autoReconnect = false,
                               std::chrono::seconds timeout = std::chrono::seconds(2))
        : output(VideoTestHelpers::createVideoOutput(engine))
        , receiver(std::make_unique<QtMultimediaVideoReceiver>())
        , stateSpy(receiver.get(), &VideoReceiver::stateChanged)
        , errorSpy(receiver.get(), &VideoReceiver::errorOccurred)
    {
        QtMultimediaVideoReceiverTest::sampleOften(*receiver);
        receiver->setName(QStringLiteral("test"));
        receiver->setConfig({.uri = uri, .timeout = timeout, .lowLatency = true, .autoReconnect = autoReconnect});
        receiver->setVideoOutput(output.get());
    }

    [[nodiscard]] bool waitForState(VideoReceiver::State state,
                                    std::chrono::milliseconds timeout = TestTimeout::longDuration()) const
    {
        return UnitTest::waitForCondition([this, state] { return receiver->state() == state; }, timeout,
                                          u"receiver state");
    }

    /// Frames reach the output and the stats measure them: a sample this short can be one without a frame.
    [[nodiscard]] bool waitForDisplay() const
    {
        return UnitTest::waitForCondition(
            [this] {
                const VideoStreamStats stats = receiver->stats();
                return receiver->decoding() && receiver->videoSize().isValid() && stats.valid && (stats.fps > 0);
            },
            TestTimeout::longDuration(), u"display");
    }

    QQmlEngine engine;
    std::unique_ptr<QQuickItem> output;
    std::unique_ptr<QtMultimediaVideoReceiver> receiver;
    QSignalSpy stateSpy;
    QSignalSpy errorSpy;
};

}  // namespace QtMultimediaTest
