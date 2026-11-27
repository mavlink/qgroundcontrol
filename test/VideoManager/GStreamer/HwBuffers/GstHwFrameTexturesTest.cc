#include "HwBuffers/GstHwFrameTexturesTest.h"

#include <array>
#include <limits>
#include <memory>
#include <rhi/qrhi.h>

#include <QtCore/QLoggingCategory>
#include <QtCore/QRegularExpression>

#include "GStreamerTestHelpers.h"
#include "GstHwPathTelemetry.h"
#include "GstHwVideoBuffer.h"

Q_LOGGING_CATEGORY(HwFrameTexturesTestLog, "Video.GStreamer.Test.HwFrameTextures")

namespace {

constexpr QSize kFrameSize(64, 64);
constexpr auto kPixelFormat = QVideoFrameFormat::Format_NV12;

/// A GPU frame whose import hands Qt @p handles as its two NV12 planes.
class HandlesTestBuffer final : public GstHwVideoBuffer
{
public:
    HandlesTestBuffer(GstSample* sample, const GstVideoInfo& info, const std::array<quint64, 2>& handles)
        : GstHwVideoBuffer(sample, info, QVideoFrameFormat(kFrameSize, kPixelFormat))
        , _planeHandles(handles)
    {}

    HwVideoBufferPath path() const override { return HwVideoBufferPath::GlMemory; }

protected:
    QVideoFrameTexturesUPtr importTextures(QRhi& rhi, QVideoFrameTexturesUPtr&) override
    {
        auto handles = std::make_unique<GstHwTextureHandles>(refSample());
        for (int plane = 0; plane < int(_planeHandles.size()); ++plane) {
            handles->setHandle(plane, _planeHandles[std::size_t(plane)]);
        }
        return wrapTextures(rhi, std::move(handles), HwFrameTexturesTestLog());
    }

private:
    std::array<quint64, 2> _planeHandles;
};

GstTest::SamplePtr makeSample(GstVideoInfo* info)
{
    return GstTest::SamplePtr(
        GstTest::makeVideoSample(GST_VIDEO_FORMAT_NV12, kFrameSize.width(), kFrameSize.height(), info));
}

}  // namespace

void GstHwFrameTexturesTest::_testHwFrameTexturesNeedEveryPlane_data()
{
    QTest::addColumn<quint64>("lumaHandle");
    QTest::addColumn<quint64>("chromaHandle");
    QTest::addColumn<bool>("wrapped");

    QTest::newRow("both planes") << quint64(11) << quint64(12) << true;
    QTest::newRow("luma handle 0") << quint64(0) << quint64(12) << false;
    QTest::newRow("chroma handle 0") << quint64(11) << quint64(0) << false;
}

void GstHwFrameTexturesTest::_testHwFrameTexturesNeedEveryPlane()
{
    QFETCH(quint64, lumaHandle);
    QFETCH(quint64, chromaHandle);
    QFETCH(bool, wrapped);

    // Each row hears of its own failures.
    GstHw::rearmOnceLatches();
    GstVideoInfo info{};
    GstTest::SamplePtr sample = makeSample(&info);
    QVERIFY(sample);
    // The Null backend's createFrom() accepts any handle, so only a missing one fails a plane.
    const std::unique_ptr<QRhi> rhi = GstTest::makeNullRhi();
    QVERIFY(rhi);
    HandlesTestBuffer frame(sample.get(), info, {lumaHandle, chromaHandle});
    const auto count = [](auto key) { return GstHwPathTelemetry::snapshot().count(HwVideoBufferPath::GlMemory, key); };
    const quint64 failuresBefore = count(GstHwPathTelemetry::Counter::MapFailures);
    const quint64 mapFailedBefore = count(GstHwPathTelemetry::HwFallbackReason::MapFailed);

    if (!wrapped) {
        expectLogMessage("Video.GStreamer.Test.HwFrameTextures", QtWarningMsg,
                         QRegularExpression(QStringLiteral("createFrom failed")));
    }
    QVideoFrameTexturesUPtr old;
    const QVideoFrameTexturesUPtr textures = frame.mapTextures(*rhi, old);
    if (!wrapped) {
        verifyExpectedLogMessage();
    }

    QCOMPARE(bool(textures), wrapped);
    QCOMPARE(count(GstHwPathTelemetry::Counter::MapFailures), failuresBefore + (wrapped ? 0 : 1));
    QCOMPARE(count(GstHwPathTelemetry::HwFallbackReason::MapFailed), mapFailedBefore + (wrapped ? 0 : 1));
    if (wrapped) {
        QVERIFY(textures->texture(0) && textures->texture(1));
        return;
    }

    // The cause is warned of once, and again once the GPU caches were reset: a new device may fail anew.
    QVERIFY(!frame.mapTextures(*rhi, old));
    GstHw::rearmOnceLatches();
    expectLogMessage("Video.GStreamer.Test.HwFrameTextures", QtWarningMsg,
                     QRegularExpression(QStringLiteral("createFrom failed")));
    QVERIFY(!frame.mapTextures(*rhi, old));
    verifyExpectedLogMessage();
}

void GstHwFrameTexturesTest::_testHwFrameTexturesKeepSampleUntilDestroyed()
{
    const std::unique_ptr<QRhi> rhi = GstTest::makeNullRhi();
    QVERIFY(rhi);
    GstVideoInfo info{};
    GstTest::SamplePtr sample = makeSample(&info);
    QVERIFY(sample);
    bool sampleReleased = false;
    GstTest::watchRelease(GST_MINI_OBJECT(sample.get()), &sampleReleased);

    auto frame = std::make_unique<HandlesTestBuffer>(sample.get(), info, std::array<quint64, 2>{11, 12});
    sample.reset();
    QVideoFrameTexturesUPtr old;
    QVideoFrameTexturesUPtr textures = frame->mapTextures(*rhi, old);
    QVERIFY(textures);
    frame.reset();
    constexpr int kPlanes = 2;
    const std::array<QRhiTexture*, kPlanes> planes{textures->texture(0), textures->texture(1)};
    QVERIFY(planes[0] && planes[1]);
    QVERIFY(!textures->texture(kPlanes));
    QVERIFY(!textures->texture(std::numeric_limits<uint>::max()));

    // Qt re-renders the current textures and calls onFrameEndInvoked() after every window frame until their slot is
    // reused.
    for (int windowFrame = 0; windowFrame < 3; ++windowFrame) {
        textures->onFrameEndInvoked();
    }
    QCOMPARE(textures->texture(0), planes[0]);
    QCOMPARE(textures->texture(1), planes[1]);
    QVERIFY2(!sampleReleased, "The textures must keep the frame's sample alive while Qt can sample them");

    textures.reset();
    QVERIFY2(sampleReleased, "Destroying the textures must release the frame's sample");
}

UT_REGISTER_TEST(GstHwFrameTexturesTest, TestLabel::Unit)
