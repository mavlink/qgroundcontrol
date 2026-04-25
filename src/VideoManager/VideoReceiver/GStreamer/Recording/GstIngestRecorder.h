#pragma once

#include "VideoRecorder.h"

class GstIngestSession;

/// Recorder that attaches a file-output branch to an already-running
/// GstIngestSession pipeline. Display keeps flowing through the existing
/// QIODevice/QtMultimedia output, so ingest-session record-while-viewing does not
/// open a second network source.
class GstIngestRecorder : public VideoRecorder
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(GstIngestRecorder)

public:
    explicit GstIngestRecorder(GstIngestSession* session, QObject* parent = nullptr);
    ~GstIngestRecorder() override;

    [[nodiscard]] bool start(const QString& path, QMediaFormat::FileFormat format) override;
    void stop() override;
    [[nodiscard]] Capabilities capabilities() const override;

private:
    GstIngestSession* _session = nullptr;
};
