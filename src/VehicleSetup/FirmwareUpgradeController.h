/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#ifndef FirmwareUpgradeController_H
#define FirmwareUpgradeController_H

#include "PX4FirmwareUpgradeThread.h"
#include "LinkManager.h"
#include "FirmwareImage.h"

#include <QObject>
#include <QUrl>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPixmap>
#include <QQuickItem>

#include "qextserialport.h"

#include <stdint.h>

/// Supported firmware types. If you modify these you will need to update the qml file as well.

// Firmware Upgrade MVC Controller for FirmwareUpgrade.qml.
class FirmwareUpgradeController : public QObject
{
    Q_OBJECT
    
public:
        typedef enum {
            AutoPilotStackPX4,
            AutoPilotStackAPM,
            PX4Flow,
            ThreeDRRadio
        } AutoPilotStackType_t;

        typedef enum {
            StableFirmware,
            BetaFirmware,
            DeveloperFirmware,
            CustomFirmware
        } FirmwareType_t;

        typedef enum {
            QuadFirmware,
            X8Firmware,
            HexaFirmware,
            OctoFirmware,
            YFirmware,
            Y6Firmware,
            HeliFirmware,
            CopterFirmware,
            PlaneFirmware,
            RoverFirmware,
            DefaultVehicleFirmware
        } FirmwareVehicleType_t;

        Q_ENUMS(AutoPilotStackType_t)
        Q_ENUMS(FirmwareType_t)
        Q_ENUMS(FirmwareVehicleType_t)

    class FirmwareIdentifier
    {
    public:
        FirmwareIdentifier(AutoPilotStackType_t stack = AutoPilotStackPX4,
                           FirmwareType_t firmware = StableFirmware,
                           FirmwareVehicleType_t vehicle = DefaultVehicleFirmware)
            : autopilotStackType(stack), firmwareType(firmware), firmwareVehicleType(vehicle) {}

        bool operator==(const FirmwareIdentifier& firmwareId) const
        {
            return (firmwareId.autopilotStackType == autopilotStackType &&
                    firmwareId.firmwareType == firmwareType &&
                    firmwareId.firmwareVehicleType == firmwareVehicleType);
        }

        // members
        AutoPilotStackType_t    autopilotStackType;
        FirmwareType_t          firmwareType;
        FirmwareVehicleType_t   firmwareVehicleType;
    };

    FirmwareUpgradeController(void);
    ~FirmwareUpgradeController();

    Q_PROPERTY(QString          boardPort                   READ boardPort                                              NOTIFY boardFound)
    Q_PROPERTY(QString          boardDescription            READ boardDescription                                       NOTIFY boardFound)
    Q_PROPERTY(QString          boardType                   MEMBER _foundBoardTypeName                                  NOTIFY boardFound)
    Q_PROPERTY(bool             pixhawkBoard                READ pixhawkBoard                                           NOTIFY boardFound)
    Q_PROPERTY(bool             px4FlowBoard                READ px4FlowBoard                                           NOTIFY boardFound)
    Q_PROPERTY(FirmwareType_t   selectedFirmwareType        READ selectedFirmwareType   WRITE setSelectedFirmwareType   NOTIFY selectedFirmwareTypeChanged)
    Q_PROPERTY(QStringList      apmAvailableVersions        READ apmAvailableVersions                                   NOTIFY apmAvailableVersionsChanged)
    Q_PROPERTY(QString          px4StableVersion            READ px4StableVersion                                       NOTIFY px4StableVersionChanged)
    Q_PROPERTY(QString          px4BetaVersion              READ px4BetaVersion                                         NOTIFY px4BetaVersionChanged)

    /// TextArea for log output
    Q_PROPERTY(QQuickItem* statusLog READ statusLog WRITE setStatusLog)
    
    /// Progress bar for you know what
    Q_PROPERTY(QQuickItem* progressBar READ progressBar WRITE setProgressBar)

    /// Starts searching for boards on the background thread
    Q_INVOKABLE void startBoardSearch(void);
    
    /// Cancels whatever state the upgrade worker thread is in
    Q_INVOKABLE void cancel(void);
    
    /// Called when the firmware type has been selected by the user to continue the flash process.
    Q_INVOKABLE void flash(AutoPilotStackType_t stackType,
                           FirmwareType_t firmwareType = StableFirmware,
                           FirmwareVehicleType_t vehicleType = DefaultVehicleFirmware );

    Q_INVOKABLE FirmwareVehicleType_t vehicleTypeFromVersionIndex(int index);
    
    // overload, not exposed to qml side
    void flash(const FirmwareIdentifier& firmwareId);

    // Property accessors
    
    QQuickItem* progressBar(void) { return _progressBar; }
    void setProgressBar(QQuickItem* progressBar) { _progressBar = progressBar; }
    
    QQuickItem* statusLog(void) { return _statusLog; }
    void setStatusLog(QQuickItem* statusLog) { _statusLog = statusLog; }
    
    QString boardPort(void) { return _foundBoardInfo.portName(); }
    QString boardDescription(void) { return _foundBoardInfo.description(); }

    FirmwareType_t selectedFirmwareType(void) { return _selectedFirmwareType; }
    void setSelectedFirmwareType(FirmwareType_t firmwareType);
    QString firmwareTypeAsString(FirmwareType_t type) const;

    QStringList apmAvailableVersions(void);
    QString px4StableVersion(void) { return _px4StableVersion; }
    QString px4BetaVersion(void) { return _px4BetaVersion; }

    bool pixhawkBoard(void) const { return _foundBoardType == QGCSerialPortInfo::BoardTypePixhawk; }
    bool px4FlowBoard(void) const { return _foundBoardType == QGCSerialPortInfo::BoardTypePX4Flow; }

signals:
    void boardFound(void);
    void noBoardFound(void);
    void boardGone(void);
    void flashComplete(void);
    void flashCancelled(void);
    void error(void);
    void selectedFirmwareTypeChanged(FirmwareType_t firmwareType);
    void apmAvailableVersionsChanged(void);
    void px4StableVersionChanged(const QString& px4StableVersion);
    void px4BetaVersionChanged(const QString& px4BetaVersion);

private slots:
    void _firmwareDownloadProgress(qint64 curr, qint64 total);
    void _firmwareDownloadFinished(QString remoteFile, QString localFile);
    void _firmwareDownloadError(QString errorMsg);
    void _foundBoard(bool firstAttempt, const QSerialPortInfo& portInfo, int boardType, QString boardName);
    void _noBoardFound(void);
    void _boardGone();
    void _foundBootloader(int bootloaderVersion, int boardID, int flashSize);
    void _error(const QString& errorString);
    void _status(const QString& statusString);
    void _bootloaderSyncFailed(void);
    void _flashComplete(void);
    void _updateProgress(int curr, int total);
    void _eraseStarted(void);
    void _eraseComplete(void);
    void _eraseProgressTick(void);
    void _apmVersionDownloadFinished(QString remoteFile, QString localFile);
    void _px4ReleasesGithubDownloadFinished(QString remoteFile, QString localFile);
    void _px4ReleasesGithubDownloadError(QString errorMsg);

private:
    void _getFirmwareFile(FirmwareIdentifier firmwareId);
    void _initFirmwareHash();
    void _downloadFirmware(void);
    void _appendStatusLog(const QString& text, bool critical = false);
    void _errorCancel(const QString& msg);
    void _loadAPMVersions(uint32_t bootloaderBoardID);
    QHash<FirmwareIdentifier, QString>* _firmwareHashForBoardId(int boardId);
    void _determinePX4StableVersion(void);

    QString _portName;
    QString _portDescription;

    // firmware hashes
    QHash<FirmwareIdentifier, QString> _rgPX4FMUV4Firmware;
    QHash<FirmwareIdentifier, QString> _rgPX4FMUV2Firmware;
    QHash<FirmwareIdentifier, QString> _rgAeroCoreFirmware;
    QHash<FirmwareIdentifier, QString> _rgPX4FMUV1Firmware;
    QHash<FirmwareIdentifier, QString> _rgMindPXFMUV2Firmware;
    QHash<FirmwareIdentifier, QString> _rgTAPV1Firmware;
    QHash<FirmwareIdentifier, QString> _rgASCV1Firmware;
    QHash<FirmwareIdentifier, QString> _rgPX4FLowFirmware;
    QHash<FirmwareIdentifier, QString> _rg3DRRadioFirmware;

    QMap<FirmwareType_t, QMap<FirmwareVehicleType_t, QString> > _apmVersionMap;
    QList<FirmwareVehicleType_t>                                _apmVehicleTypeFromCurrentVersionList;

    /// Information which comes back from the bootloader
    bool        _bootloaderFound;           ///< true: we have received the foundBootloader signals
    uint32_t    _bootloaderVersion;         ///< Bootloader version
    uint32_t    _bootloaderBoardID;         ///< Board ID
    uint32_t    _bootloaderBoardFlashSize;  ///< Flash size in bytes of board
    
    bool                 _startFlashWhenBootloaderFound;
    FirmwareIdentifier   _startFlashWhenBootloaderFoundFirmwareIdentity;

    QPixmap _boardIcon;             ///< Icon used to display image of board
    
    QString _firmwareFilename;      ///< Image which we are going to flash to the board
    
    QNetworkAccessManager*  _downloadManager;       ///< Used for firmware file downloading across the internet
    QNetworkReply*          _downloadNetworkReply;  ///< Used for firmware file downloading across the internet
    
    /// @brief Thread controller which is used to run bootloader commands on separate thread
    PX4FirmwareUpgradeThreadController* _threadController;
    
    static const int    _eraseTickMsec = 500;       ///< Progress bar update tick time for erase
    static const int    _eraseTotalMsec = 15000;    ///< Estimated amount of time erase takes
    int                 _eraseTickCount;            ///< Number of ticks for erase progress update
    QTimer              _eraseTimer;                ///< Timer used to update progress bar for erase

    static const int    _findBoardTimeoutMsec = 30000;      ///< Amount of time for user to plug in USB
    static const int    _findBootloaderTimeoutMsec = 5000;  ///< Amount time to look for bootloader
    
    QQuickItem*     _statusLog;         ///< Status log TextArea Qml control
    QQuickItem*     _progressBar;
    
    bool _searchingForBoard;    ///< true: searching for board, false: search for bootloader
    
    QSerialPortInfo                 _foundBoardInfo;
    QGCSerialPortInfo::BoardType_t  _foundBoardType;
    QString                         _foundBoardTypeName;

    FirmwareType_t                  _selectedFirmwareType;

    FirmwareImage*  _image;

    QString _px4StableVersion;  // Version strange for latest PX4 stable
    QString _px4BetaVersion;    // Version strange for latest PX4 beta
};

// global hashing function
uint qHash(const FirmwareUpgradeController::FirmwareIdentifier& firmwareId);

#endif
