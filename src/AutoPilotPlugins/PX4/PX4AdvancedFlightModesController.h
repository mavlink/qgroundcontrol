/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#ifndef PX4AdvancedFlightModesController_H
#define PX4AdvancedFlightModesController_H

#include <QObject>
#include <QQuickItem>
#include <QList>
#include <QStringList>

#include "UASInterface.h"
#include "AutoPilotPlugin.h"
#include "FactPanelController.h"

/// MVC Controller for FlightModesComponent.qml.
class PX4AdvancedFlightModesController : public FactPanelController
{
    Q_OBJECT
    
public:
    PX4AdvancedFlightModesController(void);
    
    Q_PROPERTY(bool validConfiguration MEMBER _validConfiguration CONSTANT)
    Q_PROPERTY(QString configurationErrors MEMBER _configurationErrors CONSTANT)
    
    Q_PROPERTY(int channelCount MEMBER _channelCount CONSTANT)
    Q_PROPERTY(bool fixedWing MEMBER _fixedWing CONSTANT)
    
    Q_PROPERTY(QString reservedChannels MEMBER _reservedChannels CONSTANT)
    
    Q_PROPERTY(int assistModeRow    MEMBER _assistModeRow   NOTIFY modeRowsChanged)
    Q_PROPERTY(int autoModeRow      MEMBER _autoModeRow     NOTIFY modeRowsChanged)
    Q_PROPERTY(int acroModeRow      MEMBER _acroModeRow     NOTIFY modeRowsChanged)
    Q_PROPERTY(int altCtlModeRow    MEMBER _altCtlModeRow   NOTIFY modeRowsChanged)
    Q_PROPERTY(int posCtlModeRow    MEMBER _posCtlModeRow   NOTIFY modeRowsChanged)
    Q_PROPERTY(int loiterModeRow    MEMBER _loiterModeRow   NOTIFY modeRowsChanged)
    Q_PROPERTY(int missionModeRow   MEMBER _missionModeRow  NOTIFY modeRowsChanged)
    Q_PROPERTY(int returnModeRow    MEMBER _returnModeRow   NOTIFY modeRowsChanged)
    Q_PROPERTY(int offboardModeRow  MEMBER _offboardModeRow NOTIFY modeRowsChanged)
    
    Q_PROPERTY(int manualModeChannelIndex   READ manualModeChannelIndex     WRITE setManualModeChannelIndex     NOTIFY channelIndicesChanged)
    Q_PROPERTY(int assistModeChannelIndex   READ assistModeChannelIndex                                         NOTIFY channelIndicesChanged)
    Q_PROPERTY(int autoModeChannelIndex     READ autoModeChannelIndex                                           NOTIFY channelIndicesChanged)
    Q_PROPERTY(int acroModeChannelIndex     READ acroModeChannelIndex       WRITE setAcroModeChannelIndex       NOTIFY channelIndicesChanged)
    Q_PROPERTY(int altCtlModeChannelIndex   READ altCtlModeChannelIndex                                         NOTIFY channelIndicesChanged)
    Q_PROPERTY(int posCtlModeChannelIndex   READ posCtlModeChannelIndex     WRITE setPosCtlModeChannelIndex     NOTIFY channelIndicesChanged)
    Q_PROPERTY(int loiterModeChannelIndex   READ loiterModeChannelIndex     WRITE setLoiterModeChannelIndex     NOTIFY channelIndicesChanged)
    Q_PROPERTY(int missionModeChannelIndex  READ missionModeChannelIndex                                        NOTIFY channelIndicesChanged)
    Q_PROPERTY(int returnModeChannelIndex   READ returnModeChannelIndex     WRITE setReturnModeChannelIndex     NOTIFY channelIndicesChanged)
    Q_PROPERTY(int offboardModeChannelIndex READ offboardModeChannelIndex   WRITE setOffboardModeChannelIndex   NOTIFY channelIndicesChanged)
    
    Q_PROPERTY(double manualModeRcValue     READ manualModeRcValue      NOTIFY switchLiveRangeChanged)
    Q_PROPERTY(double assistModeRcValue     READ assistModeRcValue      NOTIFY switchLiveRangeChanged)
    Q_PROPERTY(double autoModeRcValue       READ autoModeRcValue        NOTIFY switchLiveRangeChanged)
    Q_PROPERTY(double acroModeRcValue       READ acroModeRcValue        NOTIFY switchLiveRangeChanged)
    Q_PROPERTY(double altCtlModeRcValue     READ altCtlModeRcValue      NOTIFY switchLiveRangeChanged)
    Q_PROPERTY(double posCtlModeRcValue     READ posCtlModeRcValue      NOTIFY switchLiveRangeChanged)
    Q_PROPERTY(double loiterModeRcValue     READ loiterModeRcValue      NOTIFY switchLiveRangeChanged)
    Q_PROPERTY(double missionModeRcValue    READ missionModeRcValue     NOTIFY switchLiveRangeChanged)
    Q_PROPERTY(double returnModeRcValue     READ returnModeRcValue      NOTIFY switchLiveRangeChanged)
    Q_PROPERTY(double offboardModeRcValue   READ offboardModeRcValue    NOTIFY switchLiveRangeChanged)
    
    Q_PROPERTY(double manualModeThreshold   READ manualModeThreshold                                    NOTIFY thresholdsChanged)
    Q_PROPERTY(double assistModeThreshold   READ assistModeThreshold    WRITE setAssistModeThreshold    NOTIFY thresholdsChanged)
    Q_PROPERTY(double autoModeThreshold     READ autoModeThreshold      WRITE setAutoModeThreshold      NOTIFY thresholdsChanged)
    Q_PROPERTY(double acroModeThreshold     READ acroModeThreshold      WRITE setAcroModeThreshold      NOTIFY thresholdsChanged)
    Q_PROPERTY(double altCtlModeThreshold   READ altCtlModeThreshold    WRITE setAltCtlModeThreshold    NOTIFY thresholdsChanged)
    Q_PROPERTY(double posCtlModeThreshold   READ posCtlModeThreshold    WRITE setPosCtlModeThreshold    NOTIFY thresholdsChanged)
    Q_PROPERTY(double loiterModeThreshold   READ loiterModeThreshold    WRITE setLoiterModeThreshold    NOTIFY thresholdsChanged)
    Q_PROPERTY(double missionModeThreshold  READ missionModeThreshold   WRITE setMissionModeThreshold   NOTIFY thresholdsChanged)
    Q_PROPERTY(double returnModeThreshold   READ returnModeThreshold    WRITE setReturnModeThreshold    NOTIFY thresholdsChanged)
    Q_PROPERTY(double offboardModeThreshold READ offboardModeThreshold  WRITE setOffboardModeThreshold  NOTIFY thresholdsChanged)
    
    Q_PROPERTY(bool assistModeVisible   MEMBER _assistModeVisible   NOTIFY modesVisibleChanged)
    Q_PROPERTY(bool autoModeVisible     MEMBER _autoModeVisible     NOTIFY modesVisibleChanged)
    
    Q_PROPERTY(bool manualModeSelected      MEMBER _manualModeSelected      NOTIFY modesSelectedChanged)
    Q_PROPERTY(bool assistModeSelected      MEMBER _assistModeSelected      NOTIFY modesSelectedChanged)
    Q_PROPERTY(bool autoModeSelected        MEMBER _autoModeSelected        NOTIFY modesSelectedChanged)
    Q_PROPERTY(bool acroModeSelected        MEMBER _acroModeSelected        NOTIFY modesSelectedChanged)
    Q_PROPERTY(bool altCtlModeSelected      MEMBER _altCtlModeSelected      NOTIFY modesSelectedChanged)
    Q_PROPERTY(bool posCtlModeSelected      MEMBER _posCtlModeSelected      NOTIFY modesSelectedChanged)
    Q_PROPERTY(bool missionModeSelected     MEMBER _missionModeSelected     NOTIFY modesSelectedChanged)
    Q_PROPERTY(bool loiterModeSelected      MEMBER _loiterModeSelected      NOTIFY modesSelectedChanged)
    Q_PROPERTY(bool returnModeSelected      MEMBER _returnModeSelected      NOTIFY modesSelectedChanged)
    Q_PROPERTY(bool offboardModeSelected    MEMBER _offboardModeSelected    NOTIFY modesSelectedChanged)
    
    Q_PROPERTY(QStringList channelListModel MEMBER _channelListModel CONSTANT)
    
    Q_INVOKABLE void generateThresholds(void);
    
    int assistModeRow(void);
    int autoModeRow(void);
    int acroModeRow(void);
    int altCtlModeRow(void);
    int posCtlModeRow(void);
    int loiterModeRow(void);
    int missionModeRow(void);
    int returnModeRow(void);
    int offboardModeRow(void);
    
    int manualModeChannelIndex(void);
    int assistModeChannelIndex(void);
    int autoModeChannelIndex(void);
    int acroModeChannelIndex(void);
    int altCtlModeChannelIndex(void);
    int posCtlModeChannelIndex(void);
    int loiterModeChannelIndex(void);
    int missionModeChannelIndex(void);
    int returnModeChannelIndex(void);
    int offboardModeChannelIndex(void);
    
    void setManualModeChannelIndex(int index);
    void setAcroModeChannelIndex(int index);
    void setPosCtlModeChannelIndex(int index);
    void setLoiterModeChannelIndex(int index);
    void setReturnModeChannelIndex(int index);
    void setOffboardModeChannelIndex(int index);
    
    double manualModeRcValue(void);
    double assistModeRcValue(void);
    double autoModeRcValue(void);
    double acroModeRcValue(void);
    double altCtlModeRcValue(void);
    double posCtlModeRcValue(void);
    double missionModeRcValue(void);
    double loiterModeRcValue(void);
    double returnModeRcValue(void);
    double offboardModeRcValue(void);
    
    double manualModeThreshold(void);
    double assistModeThreshold(void);
    double autoModeThreshold(void);
    double acroModeThreshold(void);
    double altCtlModeThreshold(void);
    double posCtlModeThreshold(void);
    double missionModeThreshold(void);
    double loiterModeThreshold(void);
    double returnModeThreshold(void);
    double offboardModeThreshold(void);
    
    void setAssistModeThreshold(double threshold);
    void setAutoModeThreshold(double threshold);
    void setAcroModeThreshold(double threshold);
    void setAltCtlModeThreshold(double threshold);
    void setPosCtlModeThreshold(double threshold);
    void setMissionModeThreshold(double threshold);
    void setLoiterModeThreshold(double threshold);
    void setReturnModeThreshold(double threshold);
    void setOffboardModeThreshold(double threshold);
    
signals:
    void switchLiveRangeChanged(void);
    void thresholdsChanged(void);
    void modesSelectedChanged(void);
    void modesVisibleChanged(void);
    void channelIndicesChanged(void);
    void modeRowsChanged(void);
    
private slots:
    void _rcChannelsChanged(int channelCount, int pwmValues[Vehicle::cMaxRcChannels]);
    
private:
    double _switchLiveRange(const QString& param);
    void _init(void);
    void _validateConfiguration(void);
    void _recalcModeSelections(void);
    void _recalcModeRows(void);
    int _channelToChannelIndex(int channel);
    int _channelToChannelIndex(const QString& channelParam);
    int _channelIndexToChannel(int index);
    
    static const int _chanMax = 18;
    
    bool _fixedWing;
    
    double  _rcValues[_chanMax];
    int     _rgRCMin[_chanMax];
    int     _rgRCMax[_chanMax];
    bool    _rgRCReversed[_chanMax];
    
    bool    _validConfiguration;
    QString _configurationErrors;
    int     _channelCount;
    QString _reservedChannels;

    int _assistModeRow;
    int _autoModeRow;
    int _acroModeRow;
    int _altCtlModeRow;
    int _posCtlModeRow;
    int _loiterModeRow;
    int _missionModeRow;
    int _returnModeRow;
    int _offboardModeRow;
    
    bool _manualModeSelected;
    bool _assistModeSelected;
    bool _autoModeSelected;
    bool _acroModeSelected;
    bool _altCtlModeSelected;
    bool _posCtlModeSelected;
    bool _missionModeSelected;
    bool _loiterModeSelected;
    bool _returnModeSelected;
    bool _offboardModeSelected;
    
    bool _assistModeVisible;
    bool _autoModeVisible;
    
    QStringList _channelListModel;
    QList<int>  _channelListModelChannel;
};

#endif
