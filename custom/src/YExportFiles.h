/*!
 *   @brief Typhoon H QGCCorePlugin Declaration
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#pragma once

#include <QObject>
#include <QThread>

class UTMConverter;

//-----------------------------------------------------------------------------
class YExportFiles : public QThread
{
    Q_OBJECT
public:
    YExportFiles();
    ~YExportFiles();

    bool        exportData                  (bool convertToUTM);
    void        cancel                      ();

protected:
    void        run                         ();

signals:
    void        copyCompleted               (quint32 totalCount, quint32 curCount);
    void        completed                   ();
    void        message                     (QString errorMessage);
    void        cancelProcess               ();

private slots:

private:
    bool            _convertToUTM;
    bool            _cancel;
    quint32         _totalFiles;
    quint32         _curFile;

private:
    quint32     _filesInPath                (const QString& path);
    bool        _copyFilesInPath            (const QString src, const QString dst);
    bool        _convertLogsToUTM           (const QString src, const QString dst);
};
