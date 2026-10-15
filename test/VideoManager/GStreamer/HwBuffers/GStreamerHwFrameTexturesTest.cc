#include "GStreamerTest.h"

#if defined(QGC_GST_STREAMING) && defined(QGC_HAS_ANY_GPU_PATH)

#include <array>
#include <memory>
#include <rhi/qrhi.h>
#include <span>

#include <QtCore/QLoggingCategory>
#include <QtCore/QRegularExpression>

#include "GstHwFrameTexturesBase.h"
#include "GstHwPathTelemetry.h"
#include "GstHwVideoBuffer.h"

Q_LOGGING_CATEGORY(HwFrameTexturesTestLog, "Video.GStreamer.Test.HwFrameTextures")

namespace {

constexpr QSize kFrameSize(64, 64);
constexpr auto kPixelFormat = QVideoFrameFormat::Format_NV12;

class TailTestBuffer final : public GstHwVideoBuffer
{
public:
    TailTestBuffer(GstSample* sample, const GstVideoInfo& info)
        : GstHwVideoBuffer(QVideoFrame::RhiTextureHandle, sample, info, QVideoFrameFormat(kFrameSize, kPixelFormat))
    {}

    QVideoFrameTexturesUPtr mapTextures(QRhi&, QVideoFrameTexturesUPtr&) override { return {}; }

    bool holdsSample() const noexcept { return _sample != nullptr; }
};

class TailTestTextures final : public GstHwFrameTexturesBase
{
public:
    TailTestTextures(QRhi* rhi, const std::array<quint64, 2>& handles)
    {
        wrapPlanes(rhi, kFrameSize, kPixelFormat, handles);
    }

    TailTestTextures(QRhi* rhi, bool chromaRejected)
        : TailTestTextures(rhi, {11, 12})
    {
        if (chromaRejected) {
            _textures[1].reset();
        }
    }

    HwVideoBufferPath sourcePath() const override { return HwVideoBufferPath::GlMemory; }
};

struct TailFixture
{
    TailFixture()
    {
        gst_video_info_set_format(&info, GST_VIDEO_FORMAT_NV12, kFrameSize.width(), kFrameSize.height());
        sample = makeSample();
        QRhiNullInitParams params;
        rhi.reset(QRhi::create(QRhi::Null, &params));
    }

    ~TailFixture() { gst_sample_unref(sample); }

    GstSample* makeSample() const
    {
        GstBuffer* buffer = gst_buffer_new_allocate(nullptr, GST_VIDEO_INFO_SIZE(&info), nullptr);
        GstCaps* caps = gst_video_info_to_caps(&info);
        GstSample* made = gst_sample_new(buffer, caps, nullptr, nullptr);
        gst_buffer_unref(buffer);
        gst_caps_unref(caps);
        return made;
    }

    QVideoFrameTexturesUPtr map(TailTestBuffer& source, QVideoFrameTexturesUPtr& old, bool reuse, bool chromaRejected,
                                bool* built = nullptr)
    {
        const GstHwMapTail tail{.path = HwVideoBufferPath::GlMemory,
                                .frameSize = kFrameSize,
                                .pixelFormat = kPixelFormat,
                                .log = &HwFrameTexturesTestLog(),
                                .diag = &diag};
        return GstHwFrameTexturesBase::finishMap<TailTestTextures>(
            source, *rhi, old, tail, [reuse](TailTestTextures&) { return reuse; },
            [&] {
                if (built) {
                    *built = true;
                }
                return std::make_unique<TailTestTextures>(rhi.get(), chromaRejected);
            });
    }

    GstVideoInfo info{};
    GstSample* sample = nullptr;
    std::unique_ptr<QRhi> rhi;
    GstHw::MapDiagnostics diag;
};

}  // namespace

void GStreamerTest::_testHwFrameTexturesZeroHandleInvalidatesBundle_data()
{
    QTest::addColumn<quint64>("lumaHandle");
    QTest::addColumn<quint64>("chromaHandle");
    QTest::addColumn<bool>("allPlanesValid");

    QTest::newRow("both planes") << quint64(11) << quint64(12) << true;
    QTest::newRow("luma handle 0") << quint64(0) << quint64(12) << false;
    QTest::newRow("chroma handle 0") << quint64(11) << quint64(0) << false;
}

void GStreamerTest::_testHwFrameTexturesZeroHandleInvalidatesBundle()
{
    QFETCH(quint64, lumaHandle);
    QFETCH(quint64, chromaHandle);
    QFETCH(bool, allPlanesValid);

    // The Null backend's createFrom() accepts any handle, so only wrapPlanes() can reject a missing plane.
    QRhiNullInitParams params;
    const std::unique_ptr<QRhi> rhi(QRhi::create(QRhi::Null, &params));
    QVERIFY(rhi);
    const TailTestTextures textures(rhi.get(), {lumaHandle, chromaHandle});

    QCOMPARE(textures.allPlanesValid(), allPlanesValid);
}

void GStreamerTest::_testHwFrameTexturesFinishMapReusesAcceptedBundle_data()
{
    QTest::addColumn<bool>("reuse");

    QTest::newRow("reuse accepted") << true;
    QTest::newRow("reuse rejected") << false;
}

void GStreamerTest::_testHwFrameTexturesFinishMapReusesAcceptedBundle()
{
    QFETCH(bool, reuse);

    TailFixture fixture;
    QVERIFY(fixture.sample && fixture.rhi);
    TailTestBuffer firstFrame(fixture.sample, fixture.info);
    QVideoFrameTexturesUPtr none;
    QVideoFrameTexturesUPtr old = fixture.map(firstFrame, none, /*reuse=*/false, /*chromaRejected=*/false);
    QVERIFY(old);
    QVideoFrameTextures* const previous = old.get();
    (void) GstHwPathTelemetry::takeTextureReuseHits(HwVideoBufferPath::GlMemory);

    GstSample* nextSample = fixture.makeSample();
    bool nextSampleReleased = false;
    gst_mini_object_weak_ref(
        GST_MINI_OBJECT(nextSample), [](gpointer data, GstMiniObject*) { *static_cast<bool*>(data) = true; },
        &nextSampleReleased);
    auto nextFrame = std::make_unique<TailTestBuffer>(nextSample, fixture.info);
    gst_sample_unref(nextSample);
    bool built = false;
    QVideoFrameTexturesUPtr mapped = fixture.map(*nextFrame, old, reuse, /*chromaRejected=*/false, &built);

    QVERIFY(mapped);
    QCOMPARE(mapped.get() == previous, reuse);
    QCOMPARE(built, !reuse);
    QCOMPARE(GstHwPathTelemetry::takeTextureReuseHits(HwVideoBufferPath::GlMemory), quint64(reuse ? 1 : 0));
    nextFrame.reset();
    QVERIFY2(!nextSampleReleased, "The mapped bundle must keep the frame's sample alive while Qt samples it");
    mapped.reset();
    QVERIFY(nextSampleReleased);
}

void GStreamerTest::_testHwFrameTexturesFinishMapRejectsIncompleteBundle()
{
    TailFixture fixture;
    QVERIFY(fixture.sample && fixture.rhi);
    TailTestBuffer frame(fixture.sample, fixture.info);
    const quint64 failuresBefore = GstHwPathTelemetry::peekMapFailureCount(HwVideoBufferPath::GlMemory);
    const quint64 mapFailedBefore = GstHwPathTelemetry::peekFallbackReason(
        HwVideoBufferPath::GlMemory, GstHwPathTelemetry::HwFallbackReason::MapFailed);

    expectLogMessage("Video.GStreamer.Test.HwFrameTextures", QtWarningMsg,
                     QRegularExpression(QStringLiteral("createFrom failed")));
    QVideoFrameTexturesUPtr none;
    const QVideoFrameTexturesUPtr mapped = fixture.map(frame, none, /*reuse=*/false, /*chromaRejected=*/true);
    verifyExpectedLogMessage();

    QVERIFY2(!mapped, "A bundle with an unwrapped plane would render with missing chroma");
    QCOMPARE(GstHwPathTelemetry::peekMapFailureCount(HwVideoBufferPath::GlMemory), failuresBefore + 1);
    QCOMPARE(GstHwPathTelemetry::peekFallbackReason(HwVideoBufferPath::GlMemory,
                                                    GstHwPathTelemetry::HwFallbackReason::MapFailed),
             mapFailedBefore + 1);
    QVERIFY2(frame.holdsSample(), "A failed map must leave the sample for Qt's CPU fallback");
}

#else

void GStreamerTest::_testHwFrameTexturesZeroHandleInvalidatesBundle_data() {}

void GStreamerTest::_testHwFrameTexturesZeroHandleInvalidatesBundle()
{
    QSKIP("No GPU path compiled in this build");
}

void GStreamerTest::_testHwFrameTexturesFinishMapReusesAcceptedBundle_data() {}

void GStreamerTest::_testHwFrameTexturesFinishMapReusesAcceptedBundle()
{
    QSKIP("No GPU path compiled in this build");
}

void GStreamerTest::_testHwFrameTexturesFinishMapRejectsIncompleteBundle()
{
    QSKIP("No GPU path compiled in this build");
}

#endif
