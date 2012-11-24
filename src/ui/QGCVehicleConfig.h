#ifndef QGCVEHICLECONFIG_H
#define QGCVEHICLECONFIG_H

#include <QWidget>
#include <QTimer>
#include <QList>

#include "QGCToolWidget.h"
#include "UASInterface.h"

namespace Ui {
class QGCVehicleConfig;
}

class QGCVehicleConfig : public QWidget
{
    Q_OBJECT
    
public:
    explicit QGCVehicleConfig(QWidget *parent = 0);
    ~QGCVehicleConfig();

    enum RC_MODE {
        RC_MODE_1 = 1,
        RC_MODE_2 = 2,
        RC_MODE_3 = 3,
        RC_MODE_4 = 4
    };

public slots:
    /** Set the MAV currently being calibrated */
    void setActiveUAS(UASInterface* active);

    /** Start the RC calibration routine */
    void startCalibrationRC();
    /** Stop the RC calibration routine */
    void stopCalibrationRC();
    /** Start/stop the RC calibration routine */
    void toggleCalibrationRC(bool enabled);
    /** Set trim positions */
    void setTrimPositions();
    /** Detect which channels need to be inverted */
    void detectChannelInversion();
    /** Change the mode setting of the control inputs */
    void setRCModeIndex(int newRcMode);
    /** Render the data updated */
    void updateView();

    /** Set the RC channel */
    void setRollChan(int channel) {
        rcMapping[0] = channel - 1;
        updateInvertedCheckboxes(channel - 1);
    }
    /** Set the RC channel */
    void setPitchChan(int channel) {
        rcMapping[1] = channel - 1;
        updateInvertedCheckboxes(channel - 1);
    }
    /** Set the RC channel */
    void setYawChan(int channel) {
        rcMapping[2] = channel - 1;
        updateInvertedCheckboxes(channel - 1);
    }
    /** Set the RC channel */
    void setThrottleChan(int channel) {
        rcMapping[3] = channel - 1;
        updateInvertedCheckboxes(channel - 1);
    }
    /** Set the RC channel */
    void setModeChan(int channel) {
        rcMapping[4] = channel - 1;
        updateInvertedCheckboxes(channel - 1);
    }
    /** Set the RC channel */
    void setAux1Chan(int channel) {
        rcMapping[5] = channel - 1;
        updateInvertedCheckboxes(channel - 1);
    }
    /** Set the RC channel */
    void setAux2Chan(int channel) {
        rcMapping[6] = channel - 1;
        updateInvertedCheckboxes(channel - 1);
    }
    /** Set the RC channel */
    void setAux3Chan(int channel) {
        rcMapping[7] = channel - 1;
        updateInvertedCheckboxes(channel - 1);
    }

    /** Set channel inversion status */
    void setRollInverted(bool inverted) {
        rcRev[rcMapping[0]] = inverted;
    }
    /** Set channel inversion status */
    void setPitchInverted(bool inverted) {
        rcRev[rcMapping[1]] = inverted;
    }
    /** Set channel inversion status */
    void setYawInverted(bool inverted) {
        rcRev[rcMapping[2]] = inverted;
    }
    /** Set channel inversion status */
    void setThrottleInverted(bool inverted) {
        rcRev[rcMapping[3]] = inverted;
    }
    /** Set channel inversion status */
    void setModeInverted(bool inverted) {
        rcRev[rcMapping[4]] = inverted;
    }
    /** Set channel inversion status */
    void setAux1Inverted(bool inverted) {
        rcRev[rcMapping[5]] = inverted;
    }
    /** Set channel inversion status */
    void setAux2Inverted(bool inverted) {
        rcRev[rcMapping[6]] = inverted;
    }
    /** Set channel inversion status */
    void setAux3Inverted(bool inverted) {
        rcRev[rcMapping[7]] = inverted;
    }

protected slots:
    /** Reset the RC calibration */
    void resetCalibrationRC();
    /** Write the RC calibration */
    void writeCalibrationRC();
    /** Request the RC calibration */
    void requestCalibrationRC();
    /** Store all parameters in onboard EEPROM */
    void writeParameters();
    /** Receive remote control updates from MAV */
    void remoteControlChannelRawChanged(int chan, float val);
    /** Parameter changed onboard */
    void parameterChanged(int uas, int component, QString parameterName, QVariant value);
    void updateStatus(const QString& str);
    void updateError(const QString& str);
    void setRCType(int type);
    /** Check timeouts */
    void checktimeOuts();
    /** Update checkbox status */
    void updateInvertedCheckboxes(int index);

protected:
    UASInterface* mav;                  ///< The current MAV
    static const unsigned int chanMax = 8;    ///< Maximum number of channels
    unsigned int chanCount;               ///< Actual channels
    int rcType;                         ///< Type of the remote control
    quint64 rcTypeUpdateRequested;      ///< Zero if not requested, non-zero if requested
    static const unsigned int rcTypeTimeout = 5000; ///< 5 seconds timeout, in milliseconds
    float rcMin[chanMax];                 ///< Minimum values
    float rcMax[chanMax];                 ///< Maximum values
    float rcTrim[chanMax];                ///< Zero-position (center for roll/pitch/yaw, 0 throttle for throttle)
    int rcMapping[chanMax];             ///< PWM to function mappings
    float rcScaling[chanMax];           ///< Scaling of channel input to control commands
    bool rcRev[chanMax];                ///< Channel reverse
    int rcValue[chanMax];               ///< Last values
    float rcRoll;                       ///< PPM input channel used as roll control input
    float rcPitch;                      ///< PPM input channel used as pitch control input
    float rcYaw;                        ///< PPM input channel used as yaw control input
    float rcThrottle;                   ///< PPM input channel used as throttle control input
    float rcMode;                       ///< PPM input channel used as mode switch control input
    float rcAux1;                       ///< PPM input channel used as aux 1 input
    float rcAux2;                       ///< PPM input channel used as aux 1 input
    float rcAux3;                       ///< PPM input channel used as aux 1 input
    bool rcCalChanged;                  ///< Set if the calibration changes (and needs to be written)
    bool changed;                       ///< Set if any of the input data changed
    QTimer updateTimer;                 ///< Controls update intervals
    enum RC_MODE rc_mode;               ///< Mode of the remote control, according to usual convention
    QList<QGCToolWidget*> toolWidgets;  ///< Configurable widgets
    bool calibrationEnabled;            ///< calibration mode on / off
    
private:
    Ui::QGCVehicleConfig *ui;

signals:
    void visibilityChanged(bool visible);
};

#endif // QGCVEHICLECONFIG_H
