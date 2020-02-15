/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef MAVLinkLogManager_H
#define MAVLinkLogManager_H

#include <QObject>

#include "QmlObjectListModel.h"
#include "QGCLoggingCategory.h"
#include "QGCToolbox.h"
#include "Vehicle.h"

Q_DECLARE_LOGGING_CATEGORY(MAVLinkLogManagerLog)

class QNetworkAccessManager;
class MAVLinkLogManager;

//-----------------------------------------------------------------------------
class MAVLinkLogFiles : public QObject
{
    Q_OBJECT
public:
    MAVLinkLogFiles    (MAVLinkLogManager* manager, const QString& filePath, bool newFile = false);

    Q_PROPERTY(QString      name        READ    name                                CONSTANT)
    Q_PROPERTY(quint32      size        READ    size                                NOTIFY sizeChanged)
    Q_PROPERTY(bool         selected    READ    selected    WRITE setSelected       NOTIFY selectedChanged)
    Q_PROPERTY(bool         uploading   READ    uploading                           NOTIFY uploadingChanged)
    Q_PROPERTY(qreal        progress    READ    progress                            NOTIFY progressChanged)
    Q_PROPERTY(bool         writing     READ    writing                             NOTIFY writingChanged)
    Q_PROPERTY(bool         uploaded    READ    uploaded                            NOTIFY uploadedChanged)

    QString     name                () { return _name; }
    quint32     size                () { return _size; }
    bool        selected            () { return _selected; }
    bool        uploading           () { return _uploading; }
    qreal       progress            () { return _progress; }
    bool        writing             () { return _writing; }
    bool        uploaded            () { return _uploaded; }

    void        setSelected         (bool selected);
    void        setUploading        (bool uploading);
    void        setProgress         (qreal progress);
    void        setWriting          (bool writing);
    void        setSize             (quint32 size);
    void        setUploaded         (bool uploaded);

signals:
    void        sizeChanged         ();
    void        selectedChanged     ();
    void        uploadingChanged    ();
    void        progressChanged     ();
    void        writingChanged      ();
    void        uploadedChanged     ();

private:
    MAVLinkLogManager*  _manager;
    QString             _name;
    quint32             _size;
    bool                _selected;
    bool                _uploading;
    qreal               _progress;
    bool                _writing;
    bool                _uploaded;
};

//-----------------------------------------------------------------------------
class MAVLinkLogProcessor
{
public:
    MAVLinkLogProcessor();
    ~MAVLinkLogProcessor();
    void                close       ();
    bool                valid       ();
    bool                create      (MAVLinkLogManager *manager, const QString path, uint8_t id);
    MAVLinkLogFiles*    record      () { return _record; }
    QString             fileName    () { return _fileName; }
    bool                processStreamData(uint16_t _sequence, uint8_t first_message, QByteArray data);
private:
    bool                _checkSequence(uint16_t seq, int &num_drops);
    QByteArray          _writeUlogMessage(QByteArray &data);
    void                _writeData(void* data, int len);
private:
    FILE*               _fd;
    quint32             _written;
    int                 _sequence;
    int                 _numDrops;
    bool                _gotHeader;
    bool                _error;
    QByteArray          _ulogMessage;
    QString             _fileName;
    MAVLinkLogFiles*    _record;
};

//-----------------------------------------------------------------------------
class MAVLinkLogManager : public QGCTool
{
    Q_OBJECT

public:
    MAVLinkLogManager    (QGCApplication* app, QGCToolbox* toolbox);
    ~MAVLinkLogManager   ();

    Q_PROPERTY(QString              emailAddress        READ    emailAddress        WRITE setEmailAddress       NOTIFY emailAddressChanged)
    Q_PROPERTY(QString              description         READ    description         WRITE setDescription        NOTIFY descriptionChanged)
    Q_PROPERTY(QString              uploadURL           READ    uploadURL           WRITE setUploadURL          NOTIFY uploadURLChanged)
    Q_PROPERTY(QString              feedback            READ    feedback            WRITE setFeedback           NOTIFY feedbackChanged)
    Q_PROPERTY(QString              videoURL            READ    videoURL            WRITE setVideoURL           NOTIFY videoURLChanged)
    Q_PROPERTY(bool                 enableAutoUpload    READ    enableAutoUpload    WRITE setEnableAutoUpload   NOTIFY enableAutoUploadChanged)
    Q_PROPERTY(bool                 enableAutoStart     READ    enableAutoStart     WRITE setEnableAutoStart    NOTIFY enableAutoStartChanged)
    Q_PROPERTY(bool                 deleteAfterUpload   READ    deleteAfterUpload   WRITE setDeleteAfterUpload  NOTIFY deleteAfterUploadChanged)
    Q_PROPERTY(bool                 publicLog           READ    publicLog           WRITE setPublicLog          NOTIFY publicLogChanged)
    Q_PROPERTY(bool                 uploading           READ    uploading                                       NOTIFY uploadingChanged)
    Q_PROPERTY(bool                 logRunning          READ    logRunning                                      NOTIFY logRunningChanged)
    Q_PROPERTY(bool                 canStartLog         READ    canStartLog                                     NOTIFY canStartLogChanged)
    Q_PROPERTY(QmlObjectListModel*  logFiles            READ    logFiles                                        NOTIFY logFilesChanged)
    Q_PROPERTY(int                  windSpeed           READ    windSpeed           WRITE setWindSpeed          NOTIFY windSpeedChanged)
    Q_PROPERTY(QString              rating              READ    rating              WRITE setRating             NOTIFY ratingChanged)

    Q_INVOKABLE void uploadLog      ();
    Q_INVOKABLE void deleteLog      ();
    Q_INVOKABLE void cancelUpload   ();
    Q_INVOKABLE void startLogging   ();
    Q_INVOKABLE void stopLogging    ();

    QString     emailAddress        () { return _emailAddress; }
    QString     description         () { return _description; }
    QString     uploadURL           () { return _uploadURL; }
    QString     feedback            () { return _feedback; }
    QString     videoURL            () { return _videoURL; }
    bool        enableAutoUpload    () { return _enableAutoUpload; }
    bool        enableAutoStart     () { return _enableAutoStart; }
    bool        uploading                ();
    bool        logRunning          () { return _logRunning; }
    bool        canStartLog         () { return _vehicle != nullptr && !_logginDenied; }
    bool        deleteAfterUpload   () { return _deleteAfterUpload; }
    bool        publicLog           () { return _publicLog; }
    int         windSpeed           () { return _windSpeed; }
    QString     rating              () { return _rating; }
    QString     logExtension        () { return _ulogExtension; }

    QmlObjectListModel* logFiles    () { return &_logFiles; }

    void        setEmailAddress     (QString email);
    void        setDescription      (QString description);
    void        setUploadURL        (QString url);
    void        setFeedback         (QString feedback);
    void        setVideoURL         (QString url);
    void        setEnableAutoUpload (bool enable);
    void        setEnableAutoStart  (bool enable);
    void        setDeleteAfterUpload(bool enable);
    void        setWindSpeed        (int speed);
    void        setRating           (QString rate);
    void        setPublicLog        (bool publicLog);

    // Override from QGCTool
    void        setToolbox          (QGCToolbox *toolbox);

signals:
    void emailAddressChanged        ();
    void descriptionChanged         ();
    void uploadURLChanged           ();
    void feedbackChanged            ();
    void enableAutoUploadChanged    ();
    void enableAutoStartChanged     ();
    void logFilesChanged            ();
    void selectedCountChanged       ();
    void uploadingChanged           ();
    void readyRead                  (QByteArray data);
    void failed                     ();
    void succeed                    ();
    void abortUpload                ();
    void logRunningChanged          ();
    void canStartLogChanged         ();
    void deleteAfterUploadChanged   ();
    void windSpeedChanged           ();
    void ratingChanged              ();
    void videoURLChanged            ();
    void publicLogChanged           ();

private slots:
    void _uploadFinished            ();
    void _dataAvailable             ();
    void _uploadProgress            (qint64 bytesSent, qint64 bytesTotal);
    void _activeVehicleChanged      (Vehicle* vehicle);
    void _mavlinkLogData            (Vehicle* vehicle, uint8_t target_system, uint8_t target_component, uint16_t sequence, uint8_t first_message, QByteArray data, bool acked);
    void _armedChanged              (bool armed);
    void _mavCommandResult          (int vehicleId, int component, int command, int result, bool noReponseFromVehicle);

private:
    bool _sendLog                   (const QString& logFile);
    bool _processUploadResponse     (int http_code, QByteArray &data);
    bool _createNewLog              ();
    int  _getFirstSelected          ();
    void _insertNewLog              (MAVLinkLogFiles* newLog);
    void _deleteLog                 (MAVLinkLogFiles* log);
    void _discardLog                ();
    QString _makeFilename           (const QString& baseName);

private:
    QString                 _description;
    QString                 _emailAddress;
    QString                 _uploadURL;
    QString                 _feedback;
    QString                 _logPath;
    QString                 _videoURL;
    bool                    _enableAutoUpload;
    bool                    _enableAutoStart;
    QNetworkAccessManager*  _nam;
    QmlObjectListModel      _logFiles;
    MAVLinkLogFiles*        _currentLogfile;
    Vehicle*                _vehicle;
    bool                    _logRunning;
    bool                    _loggingDisabled;
    MAVLinkLogProcessor*    _logProcessor;
    bool                    _deleteAfterUpload;
    int                     _windSpeed;
    QString                 _rating;
    bool                    _publicLog;
    QString                 _ulogExtension;
    bool                    _logginDenied;

};

#endif
