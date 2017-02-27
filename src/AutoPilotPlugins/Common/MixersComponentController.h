/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/



/// @file
///     @brief Mixers Config Qml Controller
///     @author

#ifndef MixersComponentController_H
#define MixersComponentController_H

#include <QTimer>

#include "FactPanelController.h"
#include "UASInterface.h"
#include "QGCLoggingCategory.h"
#include "AutoPilotPlugin.h"

Q_DECLARE_LOGGING_CATEGORY(MixersComponentControllerLog)
Q_DECLARE_LOGGING_CATEGORY(MixersComponentControllerVerboseLog)

namespace Ui {
    class MixersComponentController;
}

class MixersComponentController : public FactPanelController
{
    Q_OBJECT
        
public:
    MixersComponentController(void);
    ~MixersComponentController();

      Q_PROPERTY(QQuickItem* getMixersCountButton MEMBER _getMixersCountButton    NOTIFY getMixersCountButtonChanged)
//    Q_PROPERTY(QQuickItem* statusText   MEMBER _statusText      NOTIFY statusTextChanged)
//    Q_PROPERTY(QQuickItem* cancelButton MEMBER _cancelButton    NOTIFY cancelButtonChanged)
//    Q_PROPERTY(QQuickItem* nextButton   MEMBER _nextButton      NOTIFY nextButtonChanged)
//    Q_PROPERTY(QQuickItem* skipButton   MEMBER _skipButton      NOTIFY skipButtonChanged)
    
//    Q_PROPERTY(int rollChannelRCValue READ rollChannelRCValue NOTIFY rollChannelRCValueChanged)
//    Q_PROPERTY(int pitchChannelRCValue READ pitchChannelRCValue NOTIFY pitchChannelRCValueChanged)
//    Q_PROPERTY(int yawChannelRCValue READ yawChannelRCValue NOTIFY yawChannelRCValueChanged)
//    Q_PROPERTY(int throttleChannelRCValue READ throttleChannelRCValue NOTIFY throttleChannelRCValueChanged)
        
//    Q_ENUMS(BindModes)
//    enum BindModes {
//        DSM2,
//        DSMX7,
//        DSMX8
//    };
    
    Q_INVOKABLE void getMixersCountButtonClicked(void);
    
    unsigned int groupValue(void);
    unsigned int mixerIndexValue(void);
    unsigned int submixerIndexValue(void);
    float parameterValue(void);
        
    
signals:
    void getMixersCountButtonChanged(void);
//    void statusTextChanged(void);
//    void nextButtonChanged(void);
//    void skipButtonChanged(void);
        
    void groupValueChanged(unsigned int groupValue);
    void mixerIndexValueChanged(unsigned int mixerValue);
    void submixerIndexValueChanged(unsigned int submixerValue);
    void parameterValueChanged(float paramValue);
        
//    /// Signalled to QML to indicate reboot is required
//    void functionMappingChangedAPMReboot(void);

private slots:
//    void _rcChannelsChanged(int channelCount, int pwmValues[Vehicle::cMaxRcChannels]);

private:
//    /// @brief A set of information associated with a mixer.
//    struct MixerInfo {
//        unsigned int        mixerType;   ///< Function mapped to this channel, rcCalFunctionMax for none
//    };

//    void _switchDetect(enum rcCalFunctions function, int channel, int value, bool moveToNextStep);
    
//    void _saveAllTrims(void);
//    bool _stickSettleComplete(int value);
//    void _validateCalibration(void);
//    void _writeParameters(void);
//    void _rcCalSaveCurrentValues(void);

//    void _setHelpImage(const char* imageFile);
    
//    void _loadSettings(void);
//    void _storeSettings(void);
    
    // Member variables

//    static const char* _imageFileMode1Dir;
//    static const char* _imageFileMode2Dir;
    
    static const int _updateInterval;   ///< Interval for ui update timer

//    static const struct FunctionInfo _rgFunctionInfoAPM[rcCalFunctionMax]; ///< Information associated with each function, PX4 firmware
//    static const struct FunctionInfo _rgFunctionInfoPX4[rcCalFunctionMax]; ///< Information associated with each function, APM firmware
    
    QQuickItem* _getMixersCountButton;
//    QQuickItem* _statusText;
//    QQuickItem* _nextButton;
//    QQuickItem* _skipButton;
    
//    QString _imageHelp;
    
//#ifdef UNITTEST_BUILD
//    // Nasty hack to expose controller to unit test code
//    static RadioComponentController*    _unitTestController;
//#endif
};

#endif // MixersComponentController_H
