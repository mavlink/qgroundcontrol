#include "SharedVideoFrame.h"

#include <memory>
#include <utility>

#include <QtMultimedia/QAbstractVideoBuffer>

namespace {

/// A frame's buffer, shared with another frame. It is mapped through the frame, which counts the mappings of both and
/// reads a picture in GPU textures back.
class SharedVideoBuffer final : public QAbstractVideoBuffer
{
public:
    explicit SharedVideoBuffer(QVideoFrame frame)
        : _frame(std::move(frame))
    {}

    MapData map(QVideoFrame::MapMode mode) override
    {
        MapData data;
        if (!_frame.map(mode)) {
            return data;
        }
        data.planeCount = _frame.planeCount();
        for (int plane = 0; plane < data.planeCount; plane++) {
            data.bytesPerLine[plane] = _frame.bytesPerLine(plane);
            data.data[plane] = _frame.bits(plane);
            data.dataSize[plane] = _frame.mappedBytes(plane);
        }
        return data;
    }

    void unmap() override { _frame.unmap(); }

    QVideoFrameFormat format() const override { return _frame.surfaceFormat(); }

private:
    QVideoFrame _frame;
};

}  // namespace

QVideoFrame SharedVideoFrame::withOwnMetadata(const QVideoFrame& frame)
{
    if (!frame.isValid()) {
        return frame;
    }
    QVideoFrame result(std::make_unique<SharedVideoBuffer>(frame));
    result.setStartTime(frame.startTime());
    result.setEndTime(frame.endTime());
    result.setRotation(frame.rotation());
    result.setMirrored(frame.mirrored());
    result.setSubtitleText(frame.subtitleText());
    return result;
}
