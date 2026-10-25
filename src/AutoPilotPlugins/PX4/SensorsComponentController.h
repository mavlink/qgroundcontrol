#pragma once

#include <QtCore/QTimer>
#include <QtQmlIntegration/QtQmlIntegration>

#include "FactPanelController.h"

/// \brief Sensors Component MVC Controller for SensorsComponent.qml.
///
class SensorsComponentController : public FactPanelController
{
    Q_OBJECT
    QML_ELEMENT
public:
    SensorsComponentController(void);

    /// Calibration state of a single orientation side shown in the preview.
    /// Values must stay in sync with the VehicleRotationCal.CalState QML enum.
    enum SideCalState {
        SideCalStateIdle,       ///< No calibration data to show (neutral preview)
        SideCalStateIncomplete, ///< Calibration running, side not yet calibrated
        SideCalStateInProgress, ///< Side actively being calibrated
        SideCalStateCompleted   ///< Side calibration complete
    };
    Q_ENUM(SideCalState)

    /// Calibration which is running through the firmware's [cal] message protocol
    enum ActiveCalibration
    {
        ActiveCalibrationNone,
        ActiveCalibrationCompass,
        ActiveCalibrationGyro,
        ActiveCalibrationAccel,
        ActiveCalibrationLevel,
        ActiveCalibrationAirspeed,
        ActiveCalibrationBaro
    };
    Q_ENUM(ActiveCalibration)

    /// How the last calibration which the page shows in its panel ended
    enum CalibrationResult
    {
        CalibrationResultNone,  ///< Running, cancelled, or none has run
        CalibrationResultSuccess,
        CalibrationResultFailed
    };
    Q_ENUM(CalibrationResult)

    Q_PROPERTY(ActiveCalibration activeCalibration READ activeCalibration NOTIFY calibrationActiveChanged)

    /// What the firmware has logged during the calibration, one message per line
    Q_PROPERTY(QString calibrationLog READ calibrationLog NOTIFY calibrationLogChanged)
    Q_PROPERTY(CalibrationResult calibrationResult READ calibrationResult NOTIFY calibrationResultChanged)
    /// The firmware's reason for a failed calibration, empty otherwise
    Q_PROPERTY(QString calibrationFailureText READ calibrationFailureText NOTIFY calibrationResultChanged)
    /// True after the vehicle was put on a side which is already calibrated, until another side is detected
    Q_PROPERTY(bool sideAlreadyDone MEMBER _sideAlreadyDone NOTIFY orientationCalSidesStateChanged)
    Q_PROPERTY(bool calibrationActive READ calibrationActive NOTIFY calibrationActiveChanged)
    /// 0 to 1, as reported by the firmware's progress messages
    Q_PROPERTY(double calibrationProgress READ calibrationProgress NOTIFY calibrationProgressChanged)
    Q_PROPERTY(bool magCalInProgress MEMBER _magCalInProgress NOTIFY calibrationActiveChanged)

    Q_PROPERTY(bool showOrientationCalArea MEMBER _showOrientationCalArea NOTIFY showOrientationCalAreaChanged)

    Q_PROPERTY(SideCalState orientationCalDownSideState MEMBER _orientationCalDownSideState NOTIFY orientationCalSidesStateChanged)
    Q_PROPERTY(SideCalState orientationCalUpsideDownSideState MEMBER _orientationCalUpsideDownSideState NOTIFY orientationCalSidesStateChanged)
    Q_PROPERTY(SideCalState orientationCalLeftSideState MEMBER _orientationCalLeftSideState NOTIFY orientationCalSidesStateChanged)
    Q_PROPERTY(SideCalState orientationCalRightSideState MEMBER _orientationCalRightSideState NOTIFY orientationCalSidesStateChanged)
    Q_PROPERTY(SideCalState orientationCalNoseDownSideState MEMBER _orientationCalNoseDownSideState NOTIFY orientationCalSidesStateChanged)
    Q_PROPERTY(SideCalState orientationCalTailDownSideState MEMBER _orientationCalTailDownSideState NOTIFY orientationCalSidesStateChanged)

    Q_PROPERTY(bool orientationCalDownSideVisible MEMBER _orientationCalDownSideVisible NOTIFY orientationCalSidesVisibleChanged)
    Q_PROPERTY(bool orientationCalUpsideDownSideVisible MEMBER _orientationCalUpsideDownSideVisible NOTIFY orientationCalSidesVisibleChanged)
    Q_PROPERTY(bool orientationCalLeftSideVisible MEMBER _orientationCalLeftSideVisible NOTIFY orientationCalSidesVisibleChanged)
    Q_PROPERTY(bool orientationCalRightSideVisible MEMBER _orientationCalRightSideVisible NOTIFY orientationCalSidesVisibleChanged)
    Q_PROPERTY(bool orientationCalNoseDownSideVisible MEMBER _orientationCalNoseDownSideVisible NOTIFY orientationCalSidesVisibleChanged)
    Q_PROPERTY(bool orientationCalTailDownSideVisible MEMBER _orientationCalTailDownSideVisible NOTIFY orientationCalSidesVisibleChanged)

    Q_PROPERTY(bool waitingForCancel MEMBER _waitingForCancel NOTIFY waitingForCancelChanged)
    /// True from the large vehicle calibration request until its result
    Q_PROPERTY(bool largeVehicleCalibrationPending READ largeVehicleCalibrationPending NOTIFY
                   largeVehicleCalibrationPendingChanged)

    Q_INVOKABLE void calibrateCompass(void);
    Q_INVOKABLE void calibrateGyro(void);
    Q_INVOKABLE void calibrateAccel(void);
    Q_INVOKABLE void calibrateLevel(void);
    Q_INVOKABLE void calibrateAirspeed(void);
    Q_INVOKABLE void calibrateBaro(void);
    /// For vehicles too large to rotate: offsets only, from a known heading and the vehicle's GPS position.
    /// largeVehicleCalibrationFinished is emitted when the firmware reports the result.
    Q_INVOKABLE void calibrateCompassLargeVehicle(double headingDegrees);
    Q_INVOKABLE void cancelCalibration(void);
    Q_INVOKABLE bool usingUDPLink(void);
    Q_INVOKABLE void resetFactoryParameters();
    /// Chip and bus of a CAL_*_ID device ID parameter value, empty when the ID is not set
    Q_INVOKABLE QString deviceDescription(int deviceId) const;

    bool calibrationActive() const { return activeCalibration() != ActiveCalibrationNone; }

    bool largeVehicleCalibrationPending() const { return _largeVehicleCalibrationPending; }

    double calibrationProgress() const { return _calibrationProgress; }

    QString calibrationLog() const { return _calibrationLog; }

    CalibrationResult calibrationResult() const { return _calibrationResult; }

    QString calibrationFailureText() const { return _calibrationFailureText; }

    ActiveCalibration activeCalibration() const;

    enum class LargeVehicleCalibrationResult
    {
        Pending,  ///< Not a result message
        Succeeded,
        Failed
    };

    /// What a status text from the vehicle says about the outcome of a large vehicle compass calibration
    static LargeVehicleCalibrationResult largeVehicleCalibrationResult(const QString& text);

    /// Why the large vehicle compass calibration command failed, from the failureCode of Vehicle::mavCommandResult
    static QString largeVehicleCommandFailureText(int failureCode);

signals:
    void showGyroCalAreaChanged(void);
    void showOrientationCalAreaChanged(void);
    void orientationCalSidesStateChanged(void);
    void orientationCalSidesVisibleChanged(void);
    void calibrationLogChanged(void);
    void calibrationResultChanged(void);
    void waitingForCancelChanged(void);
    void magCalComplete(void);
    void calibrationActiveChanged(void);
    void calibrationProgressChanged(void);
    /// A gyroscope, barometer or level horizon calibration finished. message is the firmware's failure text, empty on
    /// success.
    void inlineCalibrationFinished(bool success, const QString& message);
    /// success is false when the firmware reported a failure, message is its text
    void largeVehicleCalibrationFinished(bool success, const QString& message);
    void largeVehicleCalibrationPendingChanged(void);

private slots:
    void _handleUASTextMessage(quint32 uasId, int compId, int severity, QString text, const QString& description);
    void _handleLargeVehicleCalibrationMessage(quint32 uasId, int compId, int severity, QString text,
                                               const QString& description);
    void _handleLargeVehicleCommandResult(quint32 vehicleId, int targetComponent, int command, int ackResult,
                                          int failureCode);
    void _largeVehicleCalibrationTimedOut(void);
    void _handleParametersReset(bool success);

private:
    void _startLogCalibration(void);
    void _startVisualCalibration(void);
    void _setCalibrationProgress(double progress);
    void _appendStatusLog(const QString& text);
    void _refreshParams(void);
    void _watchLargeVehicleCalibration(void);
    void _stopWatchingLargeVehicleCalibration(void);
    void _setLargeVehicleCalibrationPending(bool pending);
    void _hideAllCalAreas(void);
    void _setAllSidesState(SideCalState state);

    enum StopCalibrationCode {
        StopCalibrationSuccess,
        StopCalibrationFailed,
        StopCalibrationCancelled
    };
    void _stopCalibration(StopCalibrationCode code);

    void _updateAndEmitShowOrientationCalArea(bool show);
    void _setCalibrationResult(CalibrationResult result);

    bool _showGyroCalArea;
    bool _showOrientationCalArea;

    bool _gyroCalInProgress;
    bool _magCalInProgress;
    bool _accelCalInProgress;
    bool _airspeedCalInProgress;
    bool _levelCalInProgress;
    bool _baroCalInProgress;

    bool _orientationCalDownSideVisible;
    bool _orientationCalUpsideDownSideVisible;
    bool _orientationCalLeftSideVisible;
    bool _orientationCalRightSideVisible;
    bool _orientationCalNoseDownSideVisible;
    bool _orientationCalTailDownSideVisible;

    SideCalState _orientationCalDownSideState;
    SideCalState _orientationCalUpsideDownSideState;
    SideCalState _orientationCalLeftSideState;
    SideCalState _orientationCalRightSideState;
    SideCalState _orientationCalNoseDownSideState;
    SideCalState _orientationCalTailDownSideState;

    bool _unknownFirmwareVersion;
    bool _waitingForCancel;
    bool _sideAlreadyDone = false;
    QString _calibrationLog;
    CalibrationResult _calibrationResult = CalibrationResultNone;
    QString _calibrationFailureText;
    QString _calibrationDetail;  ///< Last plain message the firmware logged, which gives the reason for a failure
    double _calibrationProgress = 0;

    static const int _supportedFirmwareCalVersion = 2;
    QTimer _largeVehicleCalibrationTimer;
    bool _largeVehicleCalibrationPending = false;
    static constexpr int _largeVehicleCalibrationTimeoutMs = 5000;
};
