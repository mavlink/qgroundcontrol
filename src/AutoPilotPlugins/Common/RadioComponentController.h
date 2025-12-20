#pragma once

#include <QtCore/QLoggingCategory>

#include "RemoteControlCalibrationController.h"

Q_DECLARE_LOGGING_CATEGORY(RadioComponentControllerLog)
Q_DECLARE_LOGGING_CATEGORY(RadioComponentControllerVerboseLog)

/// Controller class for RC Transmitter calibration
class RadioComponentController : public RemoteControlCalibrationController
{
    Q_OBJECT
    QML_ELEMENT

    //friend class RadioConfigTest;

public:
    RadioComponentController(QObject *parent = nullptr);
    ~RadioComponentController();

    enum BindModes {
        DSM2,
        DSMX7,
        DSMX8
    };
    Q_ENUM(BindModes)

    Q_INVOKABLE void spektrumBindMode(int mode);
    Q_INVOKABLE void crsfBindMode();

    // Overrides from RemoteControlCalibrationController
    void start() final override;

signals:
    /// Signalled to indicate cal failure due to reversed throttle
    void throttleReversedCalFailure();

private:
    QString _stickFunctionToParamName(RemoteControlCalibrationController::StickFunction function) const;
    bool _channelReversedParamValue(int channel);
    void _setChannelReversedParamValue(int channel, bool reversed);

    // Overrides from RemoteControlCalibrationController
    void _saveStoredCalibrationValues() override;
    void _readStoredCalibrationValues() override;

    QString _revParamFormat;
    bool _revParamIsBool = false;
};
