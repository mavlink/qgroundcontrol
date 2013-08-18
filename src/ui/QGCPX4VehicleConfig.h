#ifndef QGCPX4VehicleConfig_H
#define QGCPX4VehicleConfig_H

#include <QWidget>
#include <QTimer>
#include <QList>
#include <QGroupBox>
#include <QPushButton>
#include <QStringList>

#include "QGCToolWidget.h"
#include "UASInterface.h"
#include "px4_configuration/QGCPX4AirframeConfig.h"

class UASParameterCommsMgr;
class DialogBare;

namespace Ui {
class QGCPX4VehicleConfig;
}

class QGCPX4VehicleConfig : public QWidget
{
    Q_OBJECT

public:
    explicit QGCPX4VehicleConfig(QWidget *parent = 0);
    ~QGCPX4VehicleConfig();

    enum RC_MODE {
        RC_MODE_1 = 1,
        RC_MODE_2 = 2,
        RC_MODE_3 = 3,
        RC_MODE_4 = 4,
        RC_MODE_NONE = 5
    };

public slots:
    void rcMenuButtonClicked();
    void sensorMenuButtonClicked();
    void generalMenuButtonClicked();
    void advancedMenuButtonClicked();
    void airframeMenuButtonClicked();
    void firmwareMenuButtonClicked();

    void identifyChannelMapping(int aert_index);

    /** Set the MAV currently being calibrated */
    void setActiveUAS(UASInterface* active);
    /** Fallback function, automatically called by loadConfig() upon failure to find and xml file*/
    void loadQgcConfig(bool primary);
    /** Load configuration from xml file */
    void loadConfig();
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
    void updateRcWidgetValues();
    void handleRcParameterChange(QString parameterName, QVariant value);


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

    /** Identify roll */
    void identifyRollChannel() {
        identifyChannelMapping(0);
    }

    /** Identify pitch */
    void identifyPitchChannel() {
        identifyChannelMapping(1);
    }

    /** Identify yaw */
    void identifyYawChannel() {
        identifyChannelMapping(2);
    }

    /** Identify throttle */
    void identifyThrottleChannel() {
        identifyChannelMapping(3);
    }

    /** Identify mode */
    void identifyModeChannel() {
        identifyChannelMapping(4);
    }

    /** Identify sub mode */
    void identifySubModeChannel() {
        identifyChannelMapping(5);
    }

    /** Identify aux 1 */
    void identifyAux1Channel() {
        identifyChannelMapping(6);
    }

    /** Identify aux 2 */
    void identifyAux2Channel() {
        identifyChannelMapping(7);
    }

protected slots:
    void menuButtonClicked();
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
    bool doneLoadingConfig;
    UASInterface* mav;                  ///< The current MAV
    QGCUASParamManager* paramMgr;       ///< params mgr for the mav
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
    float rcMappedMin[chanMax];            ///< Mapped channels in default order
    float rcMappedMax[chanMax];            ///< Mapped channels in default order
    float rcMappedValue[chanMax];            ///< Mapped channels in default order
    int channelWanted;                  ///< During channel assignment search the requested default index
    float channelWantedList[chanMax];   ///< During channel assignment search the start values
    QStringList channelNames;           ///< List of channel names in standard order
    float rcRoll;                       ///< PPM input channel used as roll control input
    float rcPitch;                      ///< PPM input channel used as pitch control input
    float rcYaw;                        ///< PPM input channel used as yaw control input
    float rcThrottle;                   ///< PPM input channel used as throttle control input
    float rcMode;                       ///< PPM input channel used as mode switch control input
    float rcAux1;                       ///< PPM input channel used as aux 1 input
    float rcAux2;                       ///< PPM input channel used as aux 2 input
    float rcAux3;                       ///< PPM input channel used as aux 3 input
    bool rcCalChanged;                  ///< Set if the calibration changes (and needs to be written)
    bool dataModelChanged;              ///< Set if any of the input data changed
    QTimer updateTimer;                 ///< Controls update intervals
    enum RC_MODE rc_mode;               ///< Mode of the remote control, according to usual convention
    QList<QGCToolWidget*> toolWidgets;  ///< Configurable widgets
    QMap<QString,QGCToolWidget*> toolWidgetsByName; ///<
    bool calibrationEnabled;            ///< calibration mode on / off

    QMap<QString,QGCToolWidget*> paramToWidgetMap;                     ///< Holds the current active MAV's parameter widgets.
    QList<QWidget*> additionalTabs;                                   ///< Stores additional tabs loaded for this vehicle/autopilot configuration. Used for cleaning up.
    QMap<QString,QGCToolWidget*> libParamToWidgetMap;                  ///< Holds the library parameter widgets
    QMap<QString,QMap<QString,QGCToolWidget*> > systemTypeToParamMap;   ///< Holds all loaded MAV specific parameter widgets, for every MAV.
    QMap<QGCToolWidget*,QGroupBox*> toolToBoxMap;                       ///< Easy method of figuring out which QGroupBox is tied to which ToolWidget.
    QMap<QString,QString> paramTooltips;                                ///< Tooltips for the ? button next to a parameter.

    QGCPX4AirframeConfig* px4AirframeConfig;
    DialogBare* firmwareDialog;

private:
    Ui::QGCPX4VehicleConfig *ui;
    QMap<QPushButton*,QWidget*> buttonToWidgetMap;
signals:
    void visibilityChanged(bool visible);
};

#endif // QGCPX4VehicleConfig_H
