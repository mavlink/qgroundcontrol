#include "GStreamerTest.h"

#ifdef QGC_GST_STREAMING

#if defined(QGC_HAS_ANY_GPU_PATH)

#include <array>
#include <memory>
#include <rhi/qrhi.h>
#include <span>

#include <QtCore/QLoggingCategory>
#include <QtCore/QRegularExpression>

#include "GStreamerTestHelpers.h"
#include "GstHwFrameTexturesBase.h"
#include "GstHwPathTelemetry.h"
#include "GstHwVideoBuffer.h"
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH) || defined(QGC_HAS_GST_DMABUF_GPU_PATH)
#include <limits>

#include "GstGlFrameTextures.h"
#endif

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
};

class OwnFrameTextures final : public GstHwFrameTexturesBase
{
};

class ForeignFrameTextures final : public QVideoFrameTextures
{
public:
    QRhiTexture* texture(uint) const override { return nullptr; }
};

struct TailFixture
{
    TailFixture()
        : sample(makeSample(&info))
        , rhi(GstTest::makeNullRhi())
    {}

    ~TailFixture() { gst_sample_unref(sample); }

    static GstSample* makeSample(GstVideoInfo* info = nullptr)
    {
        return GstTest::makeVideoSample(GST_VIDEO_FORMAT_NV12, kFrameSize.width(), kFrameSize.height(), info);
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

void GStreamerTest::_testHwFrameTexturesRejectsForeignOldTextures()
{
    QVideoFrameTexturesUPtr foreign = std::make_unique<ForeignFrameTextures>();
    QVERIFY(!GstHwFrameTexturesBase::reusableBundle<OwnFrameTextures>(foreign));
    QVERIFY(foreign);

    QVideoFrameTexturesUPtr own = std::make_unique<OwnFrameTextures>();
    QVERIFY(GstHwFrameTexturesBase::reusableBundle<OwnFrameTextures>(own));
    QVERIFY(own);
}

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
    const std::unique_ptr<QRhi> rhi = GstTest::makeNullRhi();
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
    (void) GstHwPathTelemetry::take(HwVideoBufferPath::GlMemory, GstHwPathTelemetry::Counter::TextureReuse);

    GstSample* nextSample = TailFixture::makeSample();
    bool nextSampleReleased = false;
    GstTest::watchRelease(GST_MINI_OBJECT(nextSample), &nextSampleReleased);
    auto nextFrame = std::make_unique<TailTestBuffer>(nextSample, fixture.info);
    gst_sample_unref(nextSample);
    bool built = false;
    QVideoFrameTexturesUPtr mapped = fixture.map(*nextFrame, old, reuse, /*chromaRejected=*/false, &built);

    QVERIFY(mapped);
    QCOMPARE(mapped.get() == previous, reuse);
    QCOMPARE(built, !reuse);
    QCOMPARE(GstHwPathTelemetry::take(HwVideoBufferPath::GlMemory, GstHwPathTelemetry::Counter::TextureReuse),
             quint64(reuse ? 1 : 0));
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
    const quint64 failuresBefore =
        GstHwPathTelemetry::peek(HwVideoBufferPath::GlMemory, GstHwPathTelemetry::Counter::MapFailures);
    const quint64 mapFailedBefore = GstHwPathTelemetry::peekFallbackReason(
        HwVideoBufferPath::GlMemory, GstHwPathTelemetry::HwFallbackReason::MapFailed);

    expectLogMessage("Video.GStreamer.Test.HwFrameTextures", QtWarningMsg,
                     QRegularExpression(QStringLiteral("createFrom failed")));
    QVideoFrameTexturesUPtr none;
    const QVideoFrameTexturesUPtr mapped = fixture.map(frame, none, /*reuse=*/false, /*chromaRejected=*/true);
    verifyExpectedLogMessage();

    QVERIFY2(!mapped, "A bundle with an unwrapped plane would render with missing chroma");
    QCOMPARE(GstHwPathTelemetry::peek(HwVideoBufferPath::GlMemory, GstHwPathTelemetry::Counter::MapFailures),
             failuresBefore + 1);
    QCOMPARE(GstHwPathTelemetry::peekFallbackReason(HwVideoBufferPath::GlMemory,
                                                    GstHwPathTelemetry::HwFallbackReason::MapFailed),
             mapFailedBefore + 1);
    QVERIFY2(frame.holdsSample(), "A failed map must leave the sample for Qt's CPU fallback");
}

void GStreamerTest::_testHwFrameTexturesKeepResourcesUntilDestroyed()
{
#if !defined(QGC_HAS_GST_GLMEMORY_GPU_PATH) && !defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    QSKIP("GL-texture frame bundles not compiled in this build");
#else
    const std::unique_ptr<QRhi> rhi = GstTest::makeNullRhi();
    QVERIFY(rhi);

    GstSample* sample = GstTest::makeVideoSample(GST_VIDEO_FORMAT_NV12, 64, 64);
    QVERIFY(sample);
    bool sampleReleased = false;
    GstTest::watchRelease(GST_MINI_OBJECT(sample), &sampleReleased);

    constexpr int kPlanes = 2;
    auto bundle = std::make_unique<GstGlFrameTextures>(rhi.get(), QSize(64, 64), QVideoFrameFormat::Format_NV12,
                                                       std::array<GLuint, GstHw::kMaxPlanes>{11, 12}, kPlanes);
    bundle->setSourceSample(sample);
    std::array<QRhiTexture*, kPlanes> planes{bundle->texture(0), bundle->texture(1)};
    QVERIFY(planes[0] && planes[1]);
    QVERIFY(!bundle->texture(kPlanes));
    QVERIFY(!bundle->texture(std::numeric_limits<uint>::max()));

    // Qt re-renders the current bundle and calls onFrameEndInvoked() after every window frame until its slot is reused.
    for (int frame = 0; frame < 3; ++frame) {
        bundle->onFrameEndInvoked();
    }
    QCOMPARE(bundle->texture(0), planes[0]);
    QCOMPARE(bundle->texture(1), planes[1]);
    QVERIFY2(!sampleReleased, "Source sample must stay alive while Qt can still sample the bundle");

    bundle.reset();
    QVERIFY2(sampleReleased, "Destroying the bundle must release the source sample");
#endif
}

#else

void GStreamerTest::_testHwFrameTexturesRejectsForeignOldTextures()
{
    QSKIP("No GPU path compiled in this build");
}

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

void GStreamerTest::_testHwFrameTexturesKeepResourcesUntilDestroyed()
{
    QSKIP("No GPU path compiled in this build");
}

#endif

#endif
