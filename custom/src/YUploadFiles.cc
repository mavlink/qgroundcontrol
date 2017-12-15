/*!
 *   @brief Typhoon H QGCCorePlugin Implementation
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#include "YUploadFiles.h"
#include "QGCApplication.h"
#include "AppSettings.h"
#include "SettingsManager.h"
#include "UTMConverter.h"
#include <QDirIterator>

//-----------------------------------------------------------------------------
YUploadFiles::YUploadFiles()
    : _convertToUTM(false)
    , _convertToSkyward(false)
    , _cancel(false)
    , _totalFiles(0)
    , _curFile(0)
{

}

//-----------------------------------------------------------------------------
YUploadFiles::~YUploadFiles()
{
}

//-----------------------------------------------------------------------------
bool
YUploadFiles::exportData(bool convertToUTM, bool convertToSkyward)
{
    _convertToUTM = convertToUTM;
    _convertToSkyward = convertToSkyward;
    _totalFiles = 0;
    _curFile = 0;
    this->start(QThread::HighPriority);
    return true;
}

//-----------------------------------------------------------------------------
void
YUploadFiles::cancel()
{
    _cancel = true;
    emit cancelProcess();
}

//-----------------------------------------------------------------------------
void
YUploadFiles::run()
{
    QString telemetryPath = qgcApp()->toolbox()->settingsManager()->appSettings()->telemetrySavePath();
    QString missionPath   = qgcApp()->toolbox()->settingsManager()->appSettings()->missionSavePath();
#if defined(QT_DEBUG) && !defined (__android__)
    QString targetPath = QStringLiteral("/tmp/");
#else
    QString targetPath = QStringLiteral("/storage/sdcard1/");
#endif
    QDir destDir(targetPath);
    if (!destDir.exists()) {
        emit message(QString(tr("Target path missing. Make sure you have a (FAT32 Formatted) microSD card loaded.")));
        return;
    }
    _totalFiles = _filesInPath(telemetryPath);
    if(_convertToUTM && _convertToSkyward) {
        _totalFiles *= 3;
    } else if(_convertToUTM || _convertToSkyward) {
        _totalFiles *= 2;
    }
    _totalFiles += _filesInPath(missionPath);
    emit copyCompleted(_totalFiles, 0);
    bool ok = false;
    //-- Copy Mission Files
    emit message(QString(tr("Copying mission files...")));
    if(!_cancel && _copyFilesInPath(missionPath, QString("%1/%2").arg(targetPath).arg(AppSettings::missionDirectory))) {
        //-- Copy Telemetry Files
        emit message(QString(tr("Copying telemetry files...")));
        if(!_cancel && _copyFilesInPath(telemetryPath, QString("%1/%2").arg(targetPath).arg(AppSettings::telemetryDirectory))) {
            //-- Save UTM files
            ok = true;
            if(!_cancel && (_convertToUTM || _convertToSkyward)) {
                emit message(QString(tr("Exporting UTM telemetry files...")));
                if(!_convertLogsToUTM(telemetryPath, QString("%1/%2").arg(targetPath).arg(AppSettings::telemetryDirectory))) {
                    ok = false;
                }
            }
        }
    }
    if(_cancel) {
        emit message(QString(tr("Operation Canceled")));
    } else if(ok) {
        emit message(QString(tr("%1 files exported")).arg(_totalFiles));
        emit copyCompleted(1, 1);
    }
    emit completed();
}

//-----------------------------------------------------------------------------
quint32
YUploadFiles::_filesInPath(const QString& path)
{
    QDir destDir(path);
    QStringList fl = destDir.entryList();
    return fl.size();
}

//-----------------------------------------------------------------------------
bool
YUploadFiles::_copyFilesInPath(const QString src, const QString dst)
{
    QDir destDir(dst);
    if (!destDir.exists()) {
        if(!destDir.mkpath(".")) {
            emit message(QString(tr("Error creating destination %1")).arg(dst));
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
            emit message(QString(tr("Error copying to %1")).arg(fo.filePath()));
            return false;
        }
        emit copyCompleted(_totalFiles, ++_curFile);
    }
    return true;
}

//-----------------------------------------------------------------------------
bool
YUploadFiles::_convertLogsToUTM(const QString src, const QString dst)
{
    QDir destDir(dst);
    if (!destDir.exists()) {
        if(!destDir.mkpath(".")) {
            emit message(QString(tr("Error creating destination %1")).arg(dst));
            return false;
        }
    }
    QDirIterator it(src, QStringList() << "*.tlog", QDir::Files, QDirIterator::NoIteratorFlags);
    while(it.hasNext() && !_cancel) {
        QFileInfo fi(it.next());
        if(!_cancel && _convertToUTM) {
            QString output = dst + "/" + fi.baseName() + QStringLiteral(".utm");
            QFileInfo fo(output);
            if(fo.exists()) {
                QFile::remove(fo.filePath());
            }
            UTMConverter converter(false);
            connect(this, &YUploadFiles::cancelProcess, &converter, &UTMConverter::cancel);
            if(!converter.convertTelemetryFile(fi.filePath(), fo.filePath())) {
                emit message(QString(tr("Error exporting %1")).arg(fo.filePath()));
                return false;
            }
            emit copyCompleted(_totalFiles, ++_curFile);
        }
        if(!_cancel && _convertToSkyward) {
            QString output = dst + "/" + fi.baseName() + QStringLiteral(".sky");
            QFileInfo fo(output);
            if(fo.exists()) {
                QFile::remove(fo.filePath());
            }
            UTMConverter converter(true);
            connect(this, &YUploadFiles::cancelProcess, &converter, &UTMConverter::cancel);
            if(!converter.convertTelemetryFile(fi.filePath(), fo.filePath())) {
                emit message(QString(tr("Error exporting %1")).arg(fo.filePath()));
                return false;
            }
            emit copyCompleted(_totalFiles, ++_curFile);
        }
    }
    return true;
}
