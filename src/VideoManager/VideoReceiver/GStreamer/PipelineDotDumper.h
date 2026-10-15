#pragma once

#include <QtCore/QString>

typedef struct _GstElement GstElement;

/// Writes a receiver pipeline's graph as Graphviz .dot files.
///
/// Threading: the receiver's worker only, since a dump walks the pipeline and writes to disk.
class PipelineDotDumper
{
public:
    /// Into GST_DEBUG_DUMP_DOT_DIR as "<tag>.dot", when that is set. No-op without a pipeline.
    void dump(GstElement* pipeline, const char* tag) const;
    /// dump() plus a rotating copy under CacheLocation, so field bug reports include the topology.
    void snapshot(GstElement* pipeline, const QString& tag) const;
};
