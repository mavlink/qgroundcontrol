#include "PipelineDotDumper.h"

#include <gst/gst.h>

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QStandardPaths>
#include <QtCore/QStringList>

#include "GstScoped.h"
#include "GstVideoReceiver.h"
#include "QGCFileHelper.h"

QString PipelineDotDumper::dotFileTag(QStringView tag)
{
    // Fits a 255-byte file name with the "-yyyyMMdd-HHmmss-zzz.dot" suffix writePipelineDot() adds, even at 4 UTF-8
    // bytes a character.
    constexpr qsizetype kMaxTagLength = (255 - 24) / 4;

    QString safe;
    for (const QChar c : tag.left(kMaxTagLength)) {
        safe.append((c.isLetterOrNumber() || (c == u'-') || (c == u'_')) ? c : u'_');
    }
    return safe.isEmpty() ? QStringLiteral("graph") : safe;
}

QString PipelineDotDumper::writePipelineDot(GstElement* pipeline, QStringView tag, const QString& directory)
{
    // kMaxDotFiles: cap retained .dot pipeline graphs; below 5 makes retro-debugging hard.
    constexpr int kMaxDotFiles = 10;

    if (!pipeline || directory.isEmpty()) {
        return {};
    }
    const QDir dir(directory);

    // Rotate: remove oldest .dot files until under cap. Sort by mtime ascending.
    QFileInfoList existing =
        dir.entryInfoList(QStringList{QStringLiteral("*.dot")}, QDir::Files, QDir::Time | QDir::Reversed);
    while (existing.size() >= kMaxDotFiles) {
        QFile::remove(existing.takeFirst().absoluteFilePath());
    }

    const GStreamer::GCharPtr data =
        GStreamer::adoptGChar(gst_debug_bin_to_dot_data(GST_BIN(pipeline), kDiagnosticDotGraphDetails));
    if (!data) {
        return {};
    }
    const QString fullPath = dir.absoluteFilePath(
        QStringLiteral("%1-%2.dot")
            .arg(dotFileTag(tag), QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz"))));
    // Creates the directory when needed.
    const bool wrote = QGCFileHelper::atomicWrite(
        fullPath, QByteArray::fromRawData(data.get(), static_cast<qsizetype>(qstrlen(data.get()))));
    return wrote ? fullPath : QString{};
}

void PipelineDotDumper::setReceiverName(const QString& receiverName)
{
    _pipelineName = receiverName.isEmpty() ? QByteArrayLiteral("receiver")
                                           : (QStringLiteral("receiver-") + dotFileTag(receiverName)).toUtf8();
}

void PipelineDotDumper::dump(GstElement* pipeline, const char* tag) const
{
    // GST_DEBUG_BIN_TO_DOT_FILE is a no-op unless GST_DEBUG_DUMP_DOT_DIR is set; surface that
    // once per process so field debugging doesn't require re-reading the source.
    [[maybe_unused]] static const bool dotDirHinted =
        []() {
            if (qEnvironmentVariableIsEmpty("GST_DEBUG_DUMP_DOT_DIR")) {
                qCDebug(GstVideoReceiverLog)
                    << "Pipeline dot-graph dumps are disabled. Set GST_DEBUG_DUMP_DOT_DIR=/path/to/dir to enable.";
            }
            return true;
        }();

    if (!pipeline) {
        return;
    }
    const QByteArray fileName = _pipelineName + '-' + tag;
    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline), kDiagnosticDotGraphDetails, fileName.constData());
}

void PipelineDotDumper::snapshot(GstElement* pipeline, const QString& tag) const
{
    if (!pipeline) {
        return;
    }
    const QString safeTag = dotFileTag(tag);
    dump(pipeline, safeTag.toUtf8().constData());

    // The native dump already holds it.
    if (!qEnvironmentVariableIsEmpty("GST_DEBUG_DUMP_DOT_DIR")) {
        return;
    }
    const QString cacheRoot = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (cacheRoot.isEmpty()) {
        return;
    }
    const QString dotPath = writePipelineDot(pipeline, QString::fromUtf8(_pipelineName) + u'-' + safeTag,
                                             cacheRoot + QStringLiteral("/qgc-pipeline-dot"));
    if (!dotPath.isEmpty()) {
        qCDebug(GstVideoReceiverLog) << "Pipeline graph saved to" << dotPath;
    }
}
