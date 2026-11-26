#include "VideoStreamOutputRenderTest.h"

#include <memory>

#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtGui/QOpenGLContext>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickItemGrabResult>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGRendererInterface>
#include <QtTest/QSignalSpy>

#include "SettingsManager.h"
#include "VideoSettings.h"
#include "VideoStreamController.h"
#include "VideoTestHelpers.h"

namespace {

// A VideoStreamOutput that fills a square view, as VideoView's does.
constexpr const char* kRenderedOutputQml = R"QML(
import QtQuick
import QGroundControl.VideoManager

Item {
    id: root

    property var stream

    width: 400
    height: 400

    VideoStreamOutput {
        objectName: "output"
        anchors.fill: parent
        stream: root.stream
    }
}
)QML";

bool drawnRed(const QColor& colour)
{
    return (colour.red() > 200) && (colour.green() < 60) && (colour.blue() < 60);
}

bool drawnBlue(const QColor& colour)
{
    return (colour.red() < 60) && (colour.green() < 60) && (colour.blue() > 200);
}

}  // namespace

void VideoStreamOutputRenderTest::_testPictureIsDrawnWhereFitted_data()
{
    // For frames of 64 × 48 in a view of 400 × 400: the ratio the picture is forced to, and the rows of the view it
    // then covers, at the view's full width.
    QTest::addColumn<int>("forced");
    QTest::addColumn<qreal>("top");
    QTest::addColumn<qreal>("bottom");

    QTest::newRow("its own 4:3") << int(VideoSettings::AspectRatioAuto) << 50.0 << 350.0;
    QTest::newRow("forced to 16:9") << int(VideoSettings::AspectRatio16x9) << 87.5 << 312.5;
    QTest::newRow("forced to 1:1") << int(VideoSettings::AspectRatio1x1) << 0.0 << 400.0;
}

void VideoStreamOutputRenderTest::_testPictureIsDrawnWhereFitted()
{
    QFETCH(int, forced);
    QFETCH(qreal, top);
    QFETCH(qreal, bottom);

    VideoSettings* const videoSettings = SettingsManager::instance()->videoSettings();
    VideoTestHelpers::FactRestorer facts;
    facts.set(videoSettings->videoSource(), QString(VideoSettings::videoSourceUDPH264));
    facts.set(videoSettings->forcedAspectRatio(), forced);
    VideoStreamController stream(VideoStreamController::Role::Primary, videoSettings);
    auto* const receiver = new FakeVideoReceiver;
    stream.setReceiver(receiver);
    receiver->setDecoding(true);
    QCOMPARE(stream.forcedAspectRatio(), VideoSettings::aspectRatioOf(forced));

    QQuickWindow window;
    if (QQuickWindow::sceneGraphBackend() == QLatin1String("software")) {
        QSKIP("Qt Quick renders in software here, which draws no video");
    }
#if QT_CONFIG(opengl)
    // Qt Quick aborts when the graphics API it is to render with doesn't start.
    QOpenGLContext openGl;
    if ((QQuickWindow::graphicsApi() == QSGRendererInterface::OpenGL) && !openGl.create()) {
        QSKIP("No OpenGL here: the offscreen platform has it only with an X display");
    }
#endif
    window.resize(400, 400);
    VideoTestHelpers::InlineQmlEngine engine;
    const std::unique_ptr<QQuickItem> view =
        engine.create<QQuickItem>(kRenderedOutputQml, {{QStringLiteral("stream"), QVariant::fromValue(&stream)}});
    QVERIFY2(view, qPrintable(engine.error()));
    view->setParentItem(window.contentItem());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    // A frame whose top-left quadrant is red and the rest blue.
    VideoTestHelpers::showFrame(view->findChild<QQuickItem*>(QStringLiteral("output")),
                                VideoTestHelpers::redTopLeftQuadrant());
    // The view as Qt Quick draws it into a texture, one pixel for each of its own. Not QQuickWindow::grabWindow(): the
    // offscreen platform's window isn't on screen, and on a desktop's X server its picture often came back empty.
    QImage shown;
    const auto grab = [&view, &shown] {
        const QSharedPointer<QQuickItemGrabResult> result = view->grabToImage(QSize(400, 400));
        if (!result) {
            return false;
        }
        QSignalSpy readySpy(result.get(), &QQuickItemGrabResult::ready);
        if (!readySpy.wait(TestTimeout::shortMs())) {
            return false;
        }
        shown = result->image();
        return true;
    };
    const auto colourAt = [&shown](qreal x, qreal y) { return shown.pixelColor(qRound(x), qRound(y)); };
    const qreal height = bottom - top;
    QTRY_VERIFY_WITH_TIMEOUT(grab() && drawnRed(colourAt(100, top + (height / 4))), TestTimeout::mediumMs());

    // Its quadrants are where the picture's are.
    QVERIFY2(drawnBlue(colourAt(300, top + (height / 4))), "top right");
    QVERIFY2(drawnBlue(colourAt(100, bottom - (height / 4))), "bottom left");
    QVERIFY2(drawnBlue(colourAt(300, bottom - (height / 4))), "bottom right");

    // The picture reaches its top and bottom rows, and nothing is drawn beyond them.
    const qreal margin = 4;
    QVERIFY2(drawnRed(colourAt(100, top + margin)), "top row");
    QVERIFY2(drawnBlue(colourAt(100, bottom - margin)), "bottom row");
    if (top > margin) {
        QCOMPARE(colourAt(100, top - margin).alpha(), 0);
        QCOMPARE(colourAt(100, bottom + margin).alpha(), 0);
    }
}

UT_REGISTER_TEST(VideoStreamOutputRenderTest, TestLabel::Integration)
