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
    /** Change the mode setting of the control inputs */
    void setRCModeIndex(int newRcMode);
    /** Render the data updated */
    void updateView();

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

protected:
    UASInterface* mav;                  ///< The current MAV
    static const unsigned int chanMax = 8;    ///< Maximum number of channels
    int rcType;                         ///< Type of the remote control
    quint64 rcTypeUpdateRequested;      ///< Zero if not requested, non-zero if requested
    static const unsigned int rcTypeTimeout = 5000; ///< 5 seconds timeout, in milliseconds
    int rcMin[chanMax];                 ///< Minimum values
    int rcMax[chanMax];                 ///< Maximum values
    int rcTrim[chanMax];                ///< Zero-position (center for roll/pitch/yaw, 0 throttle for throttle)
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
    
private:
    Ui::QGCVehicleConfig *ui;

signals:
    void visibilityChanged(bool visible);
};

#endif // QGCVEHICLECONFIG_H
