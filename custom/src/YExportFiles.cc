/*!
 *   @brief Typhoon H QGCCorePlugin Implementation
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#include "YExportFiles.h"
#include "QGCApplication.h"
#include "AppSettings.h"
#include "SettingsManager.h"
#include "UTMConverter.h"
#include <QDirIterator>

//-----------------------------------------------------------------------------
YExportFiles::YExportFiles()
    : _convertToUTM(false)
    , _cancel(false)
    , _totalFiles(0)
    , _curFile(0)
{

}

//-----------------------------------------------------------------------------
YExportFiles::~YExportFiles()
{

}

//-----------------------------------------------------------------------------
bool
YExportFiles::exportData(bool convertToUTM)
{
    _convertToUTM = convertToUTM;
    _totalFiles = 0;
    _curFile = 0;
    this->start(QThread::HighPriority);
    return true;
}

//-----------------------------------------------------------------------------
void
YExportFiles::run()
{
    QString telemetryPath = qgcApp()->toolbox()->settingsManager()->appSettings()->telemetrySavePath();
    QString missionPath   = qgcApp()->toolbox()->settingsManager()->appSettings()->missionSavePath();
#if defined(QT_DEBUG) && !defined (__android__)
    QString targetPath = QStringLiteral("/tmp/");
#else
    QString targetPath = QStringLiteral("/storage/sdcard1/");
#endif
    _totalFiles = _filesInPath(telemetryPath);
    if(_convertToUTM) {
        _totalFiles *= 2;
    }
    _totalFiles += _filesInPath(missionPath);
    //-- Copy Mission Files
    if(!_cancel && _copyFilesInPath(missionPath, QString("%1/%2").arg(targetPath).arg(AppSettings::missionDirectory))) {
        //-- Copy Telemetry Files
        if(!_cancel && _copyFilesInPath(telemetryPath, QString("%1/%2").arg(targetPath).arg(AppSettings::telemetryDirectory))) {
            //-- Save UTM files
            if(!_cancel && _convertToUTM) {
                _convertLogsToUTM(telemetryPath, QString("%1/%2").arg(targetPath).arg(AppSettings::telemetryDirectory));
            }
        }
    }
    emit completed();
}

//-----------------------------------------------------------------------------
quint32
YExportFiles::_filesInPath(const QString& path)
{
    QDir destDir(path);
    QStringList fl = destDir.entryList();
    return fl.size();
}

//-----------------------------------------------------------------------------
bool
YExportFiles::_copyFilesInPath(const QString src, const QString dst)
{
    QDir destDir(dst);
    if (!destDir.exists()) {
        if(!destDir.mkpath(".")) {
            emit error(QString(tr("Error creating destination %1")).arg(dst));
            return false;
        }
    }
    QDirIterator it(src, QStringList() << "*", QDir::Files, QDirIterator::NoIteratorFlags);
    while(it.hasNext() && !_cancel) {
        QFileInfo fi(it.next());
        QString output = dst + "/" + fi.fileName();
        QFileInfo fo(output);
        if(fo.exists()) {
            QFile::remove(fo.filePath());
        }
        if(!QFile::copy(fi.filePath(), fo.filePath())) {
            emit error(QString(tr("Error copying to %1")).arg(fo.filePath()));
            return false;
        }
        emit copyCompleted(_totalFiles, ++_curFile);
    }
    return true;
}

//-----------------------------------------------------------------------------
bool
YExportFiles::_convertLogsToUTM(const QString src, const QString dst)
{
    QDir destDir(dst);
    if (!destDir.exists()) {
        if(!destDir.mkpath(".")) {
            emit error(QString(tr("Error creating destination %1")).arg(dst));
            return false;
        }
    }
    QDirIterator it(src, QStringList() << "*.tlog", QDir::Files, QDirIterator::NoIteratorFlags);
    while(it.hasNext() && !_cancel) {
        QFileInfo fi(it.next());
        QString output = dst + "/" + fi.baseName() + QStringLiteral(".utm");
        QFileInfo fo(output);
        if(fo.exists()) {
            QFile::remove(fo.filePath());
        }
        UTMConverter conv;
        if(!conv.convertTelemetryFile(fi.filePath(), fo.filePath())) {
            emit error(QString(tr("Error converting destination %1")).arg(fo.filePath()));
            return false;
        }
        emit copyCompleted(_totalFiles, ++_curFile);
    }
    return true;
}
