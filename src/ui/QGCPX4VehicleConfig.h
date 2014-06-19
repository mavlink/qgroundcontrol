#ifndef QGCPX4VehicleConfig_H
#define QGCPX4VehicleConfig_H

#include <QWidget>
#include <QTimer>
#include <QList>
#include <QGroupBox>
#include <QPushButton>
#include <QStringList>
#include <QMessageBox>
#include <QGraphicsScene>

#include "QGCToolWidget.h"
#include "UASInterface.h"
#include "px4_configuration/QGCPX4AirframeConfig.h"

class UASParameterCommsMgr;
class QGCPX4SensorCalibration;

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
    /** Start/stop the Spektrum pair routine */
    void toggleSpektrumPairing(bool enabled);
    /** Set the current trim values as attitude trim values */
    void copyAttitudeTrim();
    /** Set trim positions */
    void setTrimPositions();
    /** Detect which channels need to be inverted */
    void detectChannelInversion(int aert_index);
    /** Render the data updated */
    void updateView();

    void handleRcParameterChange(QString parameterName, QVariant value);


    /** Set the RC channel */
    void setRollChan(int channel) {
        rcMapping[0] = channel - 1;
        updateMappingView(0);
    }
    /** Set the RC channel */
    void setPitchChan(int channel) {
        rcMapping[1] = channel - 1;
        updateMappingView(1);
    }
    /** Set the RC channel */
    void setYawChan(int channel) {
        rcMapping[2] = channel - 1;
        updateMappingView(2);
    }
    /** Set the RC channel */
    void setThrottleChan(int channel) {
        rcMapping[3] = channel - 1;
        updateMappingView(3);
    }
    /** Set the RC channel */
    void setModeChan(int channel) {
        rcMapping[4] = channel - 1;
        updateMappingView(4);
    }
    /** Set the RC channel */
    void setAssistChan(int channel) {
        rcMapping[5] = channel - 1;
        updateMappingView(5);
    }
    /** Set the RC channel */
    void setMissionChan(int channel) {
        rcMapping[6] = channel - 1;
        updateMappingView(6);
    }
    /** Set the RC channel */
    void setReturnChan(int channel) {
        rcMapping[7] = channel - 1;
        updateMappingView(7);
    }
    /** Set the RC channel */
    void setFlapsChan(int channel) {
        rcMapping[8] = channel - 1;
        updateMappingView(8);
    }
    /** Set the RC channel */
    void setAux1Chan(int channel) {
        rcMapping[9] = channel - 1;
        updateMappingView(9);
    }
    /** Set the RC channel */
    void setAux2Chan(int channel) {
        rcMapping[10] = channel - 1;
        updateMappingView(10);
    }

    /** Set channel inversion status */
    void setRollInverted(bool inverted) {
        rcRev[rcMapping[0]] = inverted;
        updateMappingView(0);
    }
    /** Set channel inversion status */
    void setPitchInverted(bool inverted) {
        rcRev[rcMapping[1]] = inverted;
        updateMappingView(1);
    }
    /** Set channel inversion status */
    void setYawInverted(bool inverted) {
        rcRev[rcMapping[2]] = inverted;
        updateMappingView(2);
    }
    /** Set channel inversion status */
    void setThrottleInverted(bool inverted) {
        rcRev[rcMapping[3]] = inverted;
        updateMappingView(3);
    }
    /** Set channel inversion status */
    void setModeInverted(bool inverted) {
        rcRev[rcMapping[4]] = inverted;
        updateMappingView(4);
    }
    /** Set channel inversion status */
    void setAssistInverted(bool inverted) {
        rcRev[rcMapping[5]] = inverted;
        updateMappingView(5);
    }
    /** Set channel inversion status */
    void setMissionInverted(bool inverted) {
        rcRev[rcMapping[6]] = inverted;
        updateMappingView(6);
    }
    /** Set channel inversion status */
    void setReturnInverted(bool inverted) {
        rcRev[rcMapping[7]] = inverted;
        updateMappingView(7);
    }
    /** Set channel inversion status */
    void setFlapsInverted(bool inverted) {
        rcRev[rcMapping[8]] = inverted;
        updateMappingView(8);
    }
    /** Set channel inversion status */
    void setAux1Inverted(bool inverted) {
        rcRev[rcMapping[9]] = inverted;
        updateMappingView(9);
    }
    /** Set channel inversion status */
    void setAux2Inverted(bool inverted) {
        rcRev[rcMapping[10]] = inverted;
        updateMappingView(10);
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

    /** Identify assist channel */
    void identifyAssistChannel() {
        identifyChannelMapping(5);
    }

    /** Identify mission channel */
    void identifyMissionChannel() {
        identifyChannelMapping(6);
    }

    /** Identify return channel */
    void identifyReturnChannel() {
        identifyChannelMapping(7);
    }

    /** Identify flaps channel */
    void identifyFlapsChannel() {
        identifyChannelMapping(8);
    }

    /** Identify aux 1 */
    void identifyAux1Channel() {
        identifyChannelMapping(9);
    }

    /** Identify aux 2 */
    void identifyAux2Channel() {
        identifyChannelMapping(10);
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
    /** Check timeouts */
    void checktimeOuts();
    /** Update checkbox status */
    void updateAllInvertedCheckboxes();
    /** Update mapping view state */
    void updateMappingView(int index);
    /** Update the displayed values */
    void updateRcWidgetValues();
    /** update the channel labels */
    void updateRcChanLabels();

    QString labelForRcValue(float val) {
        return  QString("%1").arg(val, 5, 'f', 2, QChar(' '));
    }

protected:

    void setChannelToFunctionMapping(int function, int channel);

    bool doneLoadingConfig;
    UASInterface* mav;                  ///< The current MAV
    QGCUASParamManagerInterface* paramMgr;       ///< params mgr for the mav
    static const unsigned int chanMax = 18;    ///< Maximum number of channels
    static const unsigned int chanMappedMax = 18; ///< Maximum number of mapped channels (can be higher than input channel count)
    unsigned int chanCount;               ///< Actual channels
    float rcMin[chanMax];                 ///< Minimum values
    float rcMax[chanMax];                 ///< Maximum values
    float rcTrim[chanMax];                ///< Zero-position (center for roll/pitch/yaw, 0 throttle for throttle)
    int rcMapping[chanMappedMax];             ///< PWM to function mappings
    int rcToFunctionMapping[chanMax];
    float rcScaling[chanMax];           ///< Scaling of channel input to control commands
    bool rcRev[chanMax];                ///< Channel reverse
    int rcValue[chanMax];               ///< Last values, RAW
    int rcValueReversed[chanMax];            ///< Last values, accounted for reverse
    int rcMappedMin[chanMappedMax];            ///< Mapped channels in default order
    int rcMappedMax[chanMappedMax];            ///< Mapped channels in default order
    int rcMappedValue[chanMappedMax];            ///< Mapped channels in default order
    int rcMappedValueRev[chanMappedMax];
    float rcMappedNormalizedValue[chanMappedMax];            ///< Mapped channels in default order
    int channelWanted;                  ///< During channel assignment search the requested default index
    int channelReverseStateWanted;
    float channelWantedList[chanMax];   ///< During channel assignment search the start values
    float channelReverseStateWantedList[chanMax];
    QStringList channelNames;           ///< List of channel names in standard order
    float rcRoll;                       ///< PPM input channel used as roll control input
    float rcPitch;                      ///< PPM input channel used as pitch control input
    float rcYaw;                        ///< PPM input channel used as yaw control input
    float rcThrottle;                   ///< PPM input channel used as throttle control input
    float rcMode;                       ///< PPM input channel used as mode switch control input
    float rcAssist;                     ///< PPM input channel used as assist switch control input
    float rcLoiter;                     ///< PPM input channel used as loiter switch control input
    float rcReturn;                     ///< PPM input channel used as return switch control input
    float rcFlaps;                      ///< PPM input channel used as flaps control input
    float rcAux1;                       ///< PPM input channel used as aux 1 input
    float rcAux2;                       ///< PPM input channel used as aux 2 input
    bool rcCalChanged;                  ///< Set if the calibration changes (and needs to be written)
    bool dataModelChanged;              ///< Set if any of the input data changed
    QTimer updateTimer;                 ///< Controls update intervals
    QList<QGCToolWidget*> toolWidgets;  ///< Configurable widgets
    QMap<QString,QGCToolWidget*> toolWidgetsByName; ///<
    bool calibrationEnabled;            ///< calibration mode on / off
    bool configEnabled;                 ///< config mode on / off

    QMap<QString,QGCToolWidget*> paramToWidgetMap;                     ///< Holds the current active MAV's parameter widgets.
    QList<QWidget*> additionalTabs;                                   ///< Stores additional tabs loaded for this vehicle/autopilot configuration. Used for cleaning up.
    QMap<QString,QGCToolWidget*> libParamToWidgetMap;                  ///< Holds the library parameter widgets
    QMap<QString,QMap<QString,QGCToolWidget*> > systemTypeToParamMap;   ///< Holds all loaded MAV specific parameter widgets, for every MAV.
    QMap<QGCToolWidget*,QGroupBox*> toolToBoxMap;                       ///< Easy method of figuring out which QGroupBox is tied to which ToolWidget.
    QMap<QString,QString> paramTooltips;                                ///< Tooltips for the ? button next to a parameter.

    QGCPX4AirframeConfig* px4AirframeConfig;
    QPixmap planeBack;
    QPixmap planeSide;
    QGCPX4SensorCalibration* px4SensorCalibration;
    QMessageBox msgBox;
    QGraphicsScene scene;
    QPushButton* skipActionButton;

private:
    Ui::QGCPX4VehicleConfig *ui;
    QMap<QPushButton*,QWidget*> buttonToWidgetMap;
signals:
    void visibilityChanged(bool visible);
};

#endif // QGCPX4VehicleConfig_H
