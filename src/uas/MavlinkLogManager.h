/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef MavlinkLogManager_H
#define MavlinkLogManager_H

#include <QObject>

#include "QmlObjectListModel.h"
#include "QGCLoggingCategory.h"
#include "QGCToolbox.h"
#include "Vehicle.h"

Q_DECLARE_LOGGING_CATEGORY(MavlinkLogManagerLog)

class QNetworkAccessManager;
class MavlinkLogManager;

//-----------------------------------------------------------------------------
class MavlinkLogFiles : public QObject
{
    Q_OBJECT
public:
    MavlinkLogFiles    (MavlinkLogManager* manager, const QString& filePath);

    Q_PROPERTY(QString      name        READ    name                                CONSTANT)
    Q_PROPERTY(quint32      size        READ    size                                CONSTANT)
    Q_PROPERTY(bool         selected    READ    selected    WRITE setSelected       NOTIFY selectedChanged)
    Q_PROPERTY(bool         uploading   READ    uploading                           NOTIFY uploadingChanged)
    Q_PROPERTY(qreal        progress    READ    progress                            NOTIFY progressChanged)

    QString     name                () { return _name; }
    quint32     size                () { return _size; }
    bool        selected            () { return _selected; }
    bool        uploading           () { return _uploading; }
    qreal       progress            () { return _progress; }

    void        setSelected         (bool selected);
    void        setUploading        (bool uploading);
    void        setProgress         (qreal progress);

signals:
    void        selectedChanged     ();
    void        uploadingChanged    ();
    void        progressChanged     ();

private:
    MavlinkLogManager*  _manager;
    QString             _name;
    quint32             _size;
    bool                _selected;
    bool                _uploading;
    qreal               _progress;
};

class MavlinkLogManager : public QGCTool
{
    Q_OBJECT

public:
    MavlinkLogManager    (QGCApplication* app);
    ~MavlinkLogManager   ();

    Q_PROPERTY(QString              emailAddress        READ    emailAddress        WRITE setEmailAddress       NOTIFY emailAddressChanged)
    Q_PROPERTY(QString              description         READ    description         WRITE setDescription        NOTIFY descriptionChanged)
    Q_PROPERTY(QString              uploadURL           READ    uploadURL           WRITE setUploadURL          NOTIFY uploadURLChanged)
    Q_PROPERTY(bool                 enableAutoUpload    READ    enableAutoUpload    WRITE setEnableAutoUpload   NOTIFY enableAutoUploadChanged)
    Q_PROPERTY(bool                 enableAutoStart     READ    enableAutoStart     WRITE setEnableAutoStart    NOTIFY enableAutoStartChanged)
    Q_PROPERTY(bool                 busy                READ    busy                                            NOTIFY busyChanged)
    Q_PROPERTY(bool                 logRunning          READ    logRunning                                      NOTIFY logRunningChanged)
    Q_PROPERTY(bool                 canStartLog         READ    canStartLog                                     NOTIFY canStartLogChanged)
    Q_PROPERTY(QmlObjectListModel*  logFiles            READ    logFiles                                        NOTIFY logFilesChanged)

    Q_INVOKABLE void uploadLog      ();
    Q_INVOKABLE void deleteLog      ();
    Q_INVOKABLE void cancelUpload   ();
    Q_INVOKABLE void startLogging   ();
    Q_INVOKABLE void stopLogging    ();

    QString     emailAddress        () { return _emailAddress; }
    QString     description         () { return _description; }
    QString     uploadURL           () { return _uploadURL; }
    bool        enableAutoUpload    () { return _enableAutoUpload; }
    bool        enableAutoStart     () { return _enableAutoStart; }
    bool        busy                ();
    bool        logRunning          () { return _logRunning; }
    bool        canStartLog         () { return _vehicle != NULL; }

    QmlObjectListModel* logFiles    () { return &_logFiles; }

    void        setEmailAddress     (QString email);
    void        setDescription      (QString description);
    void        setUploadURL        (QString url);
    void        setEnableAutoUpload (bool enable);
    void        setEnableAutoStart  (bool enable);

    // Override from QGCTool
    void        setToolbox          (QGCToolbox *toolbox);

signals:
    void emailAddressChanged        ();
    void descriptionChanged         ();
    void uploadURLChanged           ();
    void enableAutoUploadChanged    ();
    void enableAutoStartChanged     ();
    void logFilesChanged            ();
    void selectedCountChanged       ();
    void busyChanged                ();
    void readyRead                  (QByteArray data);
    void failed                     ();
    void succeed                    ();
    void abortUpload                ();
    void logRunningChanged          ();
    void canStartLogChanged         ();

private slots:
    void _uploadFinished            ();
    void _dataAvailable             ();
    void _uploadProgress            (qint64 bytesSent, qint64 bytesTotal);
    void _activeVehicleChanged      (Vehicle* vehicle);
    void _mavlinkLogData            (Vehicle* vehicle, uint8_t target_system, uint8_t target_component, uint16_t sequence, uint8_t length, uint8_t first_message, const uint8_t* data, bool acked);
    void _armedChanged              (bool armed);

private:
    bool _sendLog                   (const QString& logFile);
    bool _processUploadResponse     (int http_code, QByteArray &data);
    bool _createNewLog              ();
    int  _getFirstSelected          ();

private:
    QString                 _description;
    QString                 _emailAddress;
    QString                 _uploadURL;
    QString                 _logPath;
    bool                    _enableAutoUpload;
    bool                    _enableAutoStart;
    QNetworkAccessManager*  _nam;
    QmlObjectListModel      _logFiles;
    MavlinkLogFiles*        _currentLogfile;
    Vehicle*                _vehicle;
    bool                    _logRunning;
    bool                    _loggingDisabled;
    FILE*                   _currentSavingFileFd;
    QString                 _currentSavingFileStr;
    uint16_t                _sequence;
};

#endif
