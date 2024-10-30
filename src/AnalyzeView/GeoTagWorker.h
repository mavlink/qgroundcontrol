/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QFileInfoList>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QString>

Q_DECLARE_LOGGING_CATEGORY(GeoTagWorkerLog)

class GeoTagWorker : public QObject
{
    Q_OBJECT

public:
    explicit GeoTagWorker(QObject *parent = nullptr);
    ~GeoTagWorker();

    QString logFile() const { return _logFile; }
    void setLogFile(const QString &logFile) { _logFile = logFile; }
    QString imageDirectory() const { return _imageDirectory; }
    void setImageDirectory(const QString &imageDirectory) { _imageDirectory = imageDirectory; }
    QString saveDirectory() const { return _saveDirectory; }
    void setSaveDirectory(const QString &saveDirectory) { _saveDirectory = saveDirectory; }

    struct CameraFeedbackPacket {
        double timestamp = 0.;
        double timestampUTC = 0.;
        uint32_t imageSequence = 0;
        double latitude = 0.;
        double longitude = 0.;
        float altitude = 0.;
        float groundDistance = 0.;
        float attitudeQuaternion[4]{};
        uint8_t captureResult = 0;
    };

signals:
    void error(const QString &errorMsg);
    void progressChanged(double progress);
    void taggingComplete();

public slots:
    bool process();
    void cancelTagging() { _cancel = true; }

private:
    bool _loadImages();
    bool _parseExif();
    bool _parseLogs();
    bool _calibrate();
    bool _tagImages();

    bool _cancel = false;
    QString _logFile;
    QString _imageDirectory;
    QString _saveDirectory;
    QFileInfoList _imageList;
    QList<double> _imageTimestamps;
    QList<CameraFeedbackPacket> _triggerList;
    QList<int> _imageIndices;
    QList<int> _imageOffsets;
    QList<int> _triggerIndices;

    static constexpr double kSteps = 5.;
};
