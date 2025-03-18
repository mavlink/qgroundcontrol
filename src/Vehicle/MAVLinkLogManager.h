/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#pragma once

#include <QtCore/QFile>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtNetwork/QHttpPart>
#include <QtQmlIntegration/QtQmlIntegration>

Q_DECLARE_LOGGING_CATEGORY(MAVLinkLogManagerLog)

class QmlObjectListModel;
class QNetworkAccessManager;
class MAVLinkLogManager;
class Vehicle;

class MAVLinkLogFiles : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString  name        READ name                            CONSTANT)
    Q_PROPERTY(bool     selected    READ selected    WRITE setSelected   NOTIFY selectedChanged)
    Q_PROPERTY(bool     uploaded    READ uploaded                        NOTIFY uploadedChanged)
    Q_PROPERTY(bool     uploading   READ uploading                       NOTIFY uploadingChanged)
    Q_PROPERTY(bool     writing     READ writing                         NOTIFY writingChanged)
    Q_PROPERTY(qreal    progress    READ progress                        NOTIFY progressChanged)
    Q_PROPERTY(quint32  size        READ size                            NOTIFY sizeChanged)

public:
    MAVLinkLogFiles(MAVLinkLogManager *manager, const QString &filePath, bool newFile = false);
    ~MAVLinkLogFiles();

    QString name() const { return _name; }
    bool selected() const { return _selected; }
    bool uploaded() const { return _uploaded; }
    bool uploading() const { return _uploading; }
    bool writing() const { return _writing; }
    qreal progress() const { return _progress; }
    quint32 size() const { return _size; }

    void setProgress(qreal progress);
    void setSelected(bool selected);
    void setSize(quint32 size);
    void setUploaded(bool uploaded);
    void setUploading(bool uploading);
    void setWriting(bool writing);

signals:
    void progressChanged();
    void selectedChanged();
    void sizeChanged();
    void uploadedChanged();
    void uploadingChanged();
    void writingChanged();

private:
    bool _selected = false;
    bool _uploaded = false;
    bool _uploading = false;
    bool _writing = false;
    qreal _progress = 0;
    QString _name;
    quint32 _size = 0;
};

/*===========================================================================*/

class MAVLinkLogProcessor
{
public:
    MAVLinkLogProcessor();
    ~MAVLinkLogProcessor();

    void close();
    bool valid() const { return ((_file.exists()) && (_record != nullptr)); }
    bool create(MAVLinkLogManager *manager, QStringView path, uint8_t id);
    MAVLinkLogFiles *record() { return _record; }
    QString fileName() const { return _fileName; }
    bool processStreamData(uint16_t _sequence, uint8_t first_message, const QByteArray &in);

private:
    bool _checkSequence(uint16_t seq, int &num_drops);
    QByteArray _writeUlogMessage(QByteArray &data);
    void _writeData(const void* data, int len);

    bool _error = false;
    bool _gotHeader = false;
    int _numDrops = 0;
    int _sequence = -1;
    MAVLinkLogFiles *_record = nullptr;
    QByteArray _ulogMessage;
    QFile _file;
    QString _fileName;
    quint32 _written = 0;

    static constexpr int kUlogMessageHeader = 3;
    static constexpr int kSequenceSize = 1 << 15;
};

/*===========================================================================*/


class MAVLinkLogManager : public QObject
{
    Q_OBJECT
    // QML_ELEMENT
    // QML_UNCREATABLE("")
    Q_MOC_INCLUDE("QmlObjectListModel.h")
    Q_PROPERTY(QString              emailAddress        READ emailAddress       WRITE setEmailAddress       NOTIFY emailAddressChanged)
    Q_PROPERTY(QString              description         READ description        WRITE setDescription        NOTIFY descriptionChanged)
    Q_PROPERTY(QString              uploadURL           READ uploadURL          WRITE setUploadURL          NOTIFY uploadURLChanged)
    Q_PROPERTY(QString              feedback            READ feedback           WRITE setFeedback           NOTIFY feedbackChanged)
    Q_PROPERTY(QString              videoURL            READ videoURL           WRITE setVideoURL           NOTIFY videoURLChanged)
    Q_PROPERTY(bool                 enableAutoUpload    READ enableAutoUpload   WRITE setEnableAutoUpload   NOTIFY enableAutoUploadChanged)
    Q_PROPERTY(bool                 enableAutoStart     READ enableAutoStart    WRITE setEnableAutoStart    NOTIFY enableAutoStartChanged)
    Q_PROPERTY(bool                 deleteAfterUpload   READ deleteAfterUpload  WRITE setDeleteAfterUpload  NOTIFY deleteAfterUploadChanged)
    Q_PROPERTY(bool                 publicLog           READ publicLog          WRITE setPublicLog          NOTIFY publicLogChanged)
    Q_PROPERTY(bool                 uploading           READ uploading                                      NOTIFY uploadingChanged)
    Q_PROPERTY(bool                 logRunning          READ logRunning                                     NOTIFY logRunningChanged)
    Q_PROPERTY(bool                 canStartLog         READ canStartLog                                    NOTIFY canStartLogChanged)
    Q_PROPERTY(QmlObjectListModel   *logFiles           READ logFiles                                       NOTIFY logFilesChanged)
    Q_PROPERTY(int                  windSpeed           READ windSpeed          WRITE setWindSpeed          NOTIFY windSpeedChanged)
    Q_PROPERTY(QString              rating              READ rating             WRITE setRating             NOTIFY ratingChanged)

public:
    /// Constructs an MAVLinkLogManager object.
    ///     @param parent The parent QObject.
    explicit MAVLinkLogManager(Vehicle *vehicle, QObject *parent = nullptr);

    /// Destructor for the MAVLinkLogManager class.
    ~MAVLinkLogManager();

    Q_INVOKABLE void cancelUpload();
    Q_INVOKABLE void deleteLog();
    Q_INVOKABLE void startLogging();
    Q_INVOKABLE void stopLogging();
    Q_INVOKABLE void uploadLog();

    QString emailAddress() const { return _emailAddress; }
    QString description() const { return _description; }
    QString uploadURL() const { return _uploadURL; }
    QString feedback() const { return _feedback; }
    QString videoURL() const { return _videoURL; }
    bool enableAutoUpload() const { return _enableAutoUpload; }
    bool enableAutoStart() const { return _enableAutoStart; }
    bool uploading() const { return (_currentLogfile != nullptr); }
    bool logRunning() const { return _logRunning; }
    bool canStartLog() const { return !_loggingDenied; }
    bool deleteAfterUpload() const { return _deleteAfterUpload; }
    bool publicLog() const { return _publicLog; }
    int windSpeed() const { return _windSpeed; }
    QString rating() const { return _rating; }
    QString logExtension() const { return _ulogExtension; }

    QmlObjectListModel *logFiles() { return _logFiles; }

    void setDeleteAfterUpload(bool enable);
    void setDescription(const QString &description);
    void setEmailAddress(const QString &email);
    void setEnableAutoStart(bool enable);
    void setEnableAutoUpload(bool enable);
    void setFeedback(const QString &feedback);
    void setPublicLog(bool publicLog);
    void setRating(const QString &rate);
    void setUploadURL(const QString &url);
    void setVideoURL(const QString &url);
    void setWindSpeed(int speed);

signals:
    void abortUpload();
    void canStartLogChanged();
    void deleteAfterUploadChanged();
    void descriptionChanged();
    void emailAddressChanged();
    void enableAutoStartChanged();
    void enableAutoUploadChanged();
    void failed();
    void feedbackChanged();
    void logFilesChanged();
    void logRunningChanged();
    void publicLogChanged();
    void ratingChanged();
    void readyRead(const QByteArray &data);
    void selectedCountChanged();
    void succeed();
    void uploadingChanged();
    void uploadURLChanged();
    void videoURLChanged();
    void windSpeedChanged();

private slots:
    void _uploadFinished();
    void _dataAvailable();
    void _uploadProgress(qint64 bytesSent, qint64 bytesTotal);
    void _mavlinkLogData(Vehicle *vehicle, uint8_t target_system, uint8_t target_component, uint16_t sequence, uint8_t first_message, const QByteArray &data, bool acked);
    void _armedChanged(bool armed);
    void _mavCommandResult(int vehicleId, int component, int command, int result, bool noReponseFromVehicle);

private:
    bool _sendLog(const QString &logFile);
    bool _processUploadResponse(int http_code, const QByteArray &data);
    bool _createNewLog();
    int  _getFirstSelected() const;
    void _insertNewLog(MAVLinkLogFiles *newLog);
    void _deleteLog(MAVLinkLogFiles *log);
    void _discardLog();
    QString _makeFilename(const QString &baseName) const;

    static QHttpPart _createFormPart(QStringView name, QStringView value);

    Vehicle *_vehicle = nullptr;
    QNetworkAccessManager *_networkManager = nullptr;
    QmlObjectListModel *_logFiles = nullptr;
    QString _ulogExtension;
    QString _logPath;

    bool _deleteAfterUpload = false;
    bool _enableAutoStart = false;
    bool _enableAutoUpload = true;
    bool _loggingDisabled = false;
    bool _loggingDenied = false;
    bool _logRunning = false;
    bool _publicLog = false;
    int _windSpeed = -1;
    MAVLinkLogFiles *_currentLogfile = nullptr;
    MAVLinkLogProcessor *_logProcessor = nullptr;
    QString _description;
    QString _emailAddress;
    QString _feedback;
    QString _rating;
    QString _uploadURL;
    QString _videoURL;

    static constexpr const char *kMAVLinkLogGroup = "MAVLinkLogGroup";
    static constexpr const char *kEmailAddressKey = "Email";
    static constexpr const char *kDescriptionsKey = "Description";
    static constexpr const char *kDefaultDescr = "QGroundControl Session";
    static constexpr const char *kPx4URLKey = "LogURL";
    static constexpr const char *kDefaultPx4URL = "https://logs.px4.io/upload";
    static constexpr const char *kEnableAutoUploadKey = "EnableAutoUpload";
    static constexpr const char *kEnableAutoStartKey = "EnableAutoStart";
    static constexpr const char *kEnableDeletetKey = "EnableDelete";
    static constexpr const char *kVideoURLKey = "VideoURL";
    static constexpr const char *kWindSpeedKey = "WindSpeed";
    static constexpr const char *kRateKey = "RateKey";
    static constexpr const char *kPublicLogKey = "PublicLog";
    static constexpr const char *kFeedback = "feedback";
    static constexpr const char *kVideoURL = "videoUrl";
};
