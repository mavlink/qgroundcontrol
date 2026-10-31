#pragma once

#include <gst/gstdebugutils.h>

#include <QtCore/QByteArray>
#include <QtCore/QString>

/// Writes a receiver pipeline's graph as Graphviz .dot files.
///
/// Threading: the receiver's worker only, since a dump walks the pipeline and writes to disk.
class PipelineDotDumper
{
public:
    /// Every pipeline graph QGC writes omits element properties, because source properties can contain credentials.
    static constexpr GstDebugGraphDetails kDiagnosticDotGraphDetails = static_cast<GstDebugGraphDetails>(
        GST_DEBUG_GRAPH_SHOW_MEDIA_TYPE | GST_DEBUG_GRAPH_SHOW_CAPS_DETAILS | GST_DEBUG_GRAPH_SHOW_STATES);

    /// Returns @p tag as a file-name part: letters, digits, '-' and '_' only (anything else becomes '_'), at most 57
    /// characters, never empty. A dump tag can then neither leave its directory nor make the file name too long.
    static QString dotFileTag(QStringView tag);

    /// Writes @p pipeline's graph (kDiagnosticDotGraphDetails) into @p directory as "<dotFileTag(tag)>-<UTC time>.dot",
    /// keeping at most 10 .dot files there. Returns the file's path, or empty on failure.
    static QString writePipelineDot(GstElement* pipeline, QStringView tag, const QString& directory);

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
