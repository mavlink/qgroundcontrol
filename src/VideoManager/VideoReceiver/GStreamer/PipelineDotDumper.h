#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QString>

typedef struct _GstElement GstElement;

/// Writes a receiver pipeline's graph as Graphviz .dot files. Every dump leaves out element properties, since source
/// properties can hold stream credentials.
///
/// Threading: the receiver's worker only, since a dump walks the pipeline and writes to disk.
class PipelineDotDumper
{
public:
    /// Names the pipeline, and so its dump files, "receiver-<receiverName>". VideoManager names each receiver
    /// uniquely, so receivers don't overwrite each other's dumps.
    void setReceiverName(const QString& receiverName);

    const QByteArray& pipelineName() const { return _pipelineName; }

    /// Into GST_DEBUG_DUMP_DOT_DIR as "<pipeline name>-<tag>.dot", when that is set. No-op without a pipeline.
    void dump(GstElement* pipeline, const char* tag) const;
    /// For bug reports: dump(), or a rotating copy under CacheLocation when GST_DEBUG_DUMP_DOT_DIR isn't set.
    /// @p tag may come from the user; it is reduced to a safe file-name part.
    void snapshot(GstElement* pipeline, const QString& tag) const;

private:
    QByteArray _pipelineName = QByteArrayLiteral("receiver");
};
