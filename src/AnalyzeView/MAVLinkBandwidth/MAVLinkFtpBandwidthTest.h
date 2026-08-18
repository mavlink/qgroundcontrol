#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QElapsedTimer>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/QTemporaryDir>

class FTPManager;
class Vehicle;

class MAVLinkFtpBandwidthTest : public QObject
{
    Q_OBJECT

public:
    enum class Direction
    {
        QgcToVehicle,
        VehicleToQgc,
    };

    explicit MAVLinkFtpBandwidthTest(Vehicle* vehicle, QObject* parent = nullptr);
    ~MAVLinkFtpBandwidthTest() override;

    [[nodiscard]] bool start(Direction direction, int fileSizeKiB);
    void cancel(const QString& statusText = QString());
    bool active() const;

signals:
    void phaseChanged(const QString& phaseText);
    void preparationProgress(float progress);
    void measurementStarted();
    void measurementProgress(float progress);
    void measurementComplete(qint64 elapsedMs, quint64 payloadBytes);
    void finished(bool success, const QString& statusText);

private slots:
    void _uploadComplete(const QString& remotePath, const QString& errorMessage);
    void _downloadComplete(const QString& localPath, const QString& errorMessage);
    void _deleteComplete(const QString& remotePath, const QString& errorMessage);
    void _commandProgress(float progress);

private:
    enum class Phase
    {
        Idle,
        PreparingDownload,
        MeasuringUpload,
        VerifyingUpload,
        MeasuringDownload,
        Cleaning,
    };

    [[nodiscard]] bool _createPayloadFile(int fileSizeKiB);
    [[nodiscard]] QByteArray _fileHash(const QString& filePath) const;
    [[nodiscard]] bool _startUpload();
    [[nodiscard]] bool _startDownload(const QString& fileName);
    void _completeMeasurement();
    void _beginCleanup(bool success, const QString& statusText);
    void _finish(bool success, const QString& statusText);

    QPointer<Vehicle> _vehicle;
    QPointer<FTPManager> _ftpManager;
    QTemporaryDir _temporaryDirectory;
    QElapsedTimer _measurementTimer;
    Direction _direction = Direction::QgcToVehicle;
    Phase _phase = Phase::Idle;
    QString _sourceFilePath;
    QString _downloadFileName;
    QString _remoteFilePath;
    QString _pendingStatusText;
    QString _cancelStatusText;
    QByteArray _expectedHash;
    quint64 _payloadBytes = 0;
    bool _pendingSuccess = false;
    bool _cancelled = false;

    static constexpr int kMinimumFileSizeKiB = 64;
    static constexpr int kMaximumFileSizeKiB = 10 * 1024;
};
