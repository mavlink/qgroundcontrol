/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QLoggingCategory>
#include <QtQmlIntegration/QtQmlIntegration>

#include "QmlObjectListModel.h"

Q_DECLARE_LOGGING_CATEGORY(LogDownloadControllerLog)

class Vehicle;
class QGCLogEntry;
struct LogDownloadData;

//-----------------------------------------------------------------------------
class LogDownloadController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_MOC_INCLUDE("Vehicle.h")

public:
    LogDownloadController(void);

    Q_PROPERTY(QmlObjectListModel* model    READ model              NOTIFY modelChanged)
    Q_PROPERTY(bool         requestingList  READ requestingList     NOTIFY requestingListChanged)
    Q_PROPERTY(bool         downloadingLogs READ downloadingLogs    NOTIFY downloadingLogsChanged)

    QmlObjectListModel* model           () { return &_logEntriesModel; }
    bool                requestingList  () const{ return _requestingLogEntries; }
    bool                downloadingLogs () const{ return _downloadingLogs; }

    Q_INVOKABLE void refresh                ();
    Q_INVOKABLE void download               (QString path = QString());
    Q_INVOKABLE void eraseAll               ();
    Q_INVOKABLE void cancel                 ();

    void downloadToDirectory(const QString& dir);

signals:
    void requestingListChanged  ();
    void downloadingLogsChanged ();
    void modelChanged           ();
    void selectionChanged       ();

private slots:
    void _setActiveVehicle  (Vehicle* vehicle);
    void _logEntry          (uint32_t time_utc, uint32_t size, uint16_t id, uint16_t num_logs, uint16_t last_log_num);
    void _logData           (uint32_t ofs, uint16_t id, uint8_t count, const uint8_t *data);
    void _processDownload   ();

private:
    bool _entriesComplete   ();
    bool _chunkComplete     () const;
    bool _logComplete       () const;
    void _findMissingEntries();
    void _receivedAllEntries();
    void _receivedAllData   ();
    void _resetSelection    (bool canceled = false);
    void _findMissingData   ();
    void _requestLogList    (uint32_t start, uint32_t end);
    void _requestLogData    (uint16_t id, uint32_t offset, uint32_t count, int retryCount = 0);
    bool _prepareLogDownload();
    void _setDownloading    (bool active);
    void _setListing        (bool active);
    void _updateDataRate    ();

    QGCLogEntry* _getNextSelected();

    LogDownloadData*    _downloadData;
    QTimer              _timer;
    QmlObjectListModel  _logEntriesModel;
    Vehicle*            _vehicle;
    bool                _requestingLogEntries;
    bool                _downloadingLogs;
    int                 _retries;
    int                 _apmOneBased;
    QString             _downloadPath;
};
