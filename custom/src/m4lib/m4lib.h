#pragma once

#include "TyphoonHCommon.h"
#include "m4serial.h"
#include "m4util.h"

#include <functional>


class TimerInterface {
public:
    virtual void start(int time_ms) = 0;
    virtual void stop() = 0;
    virtual void setCallback(std::function<void()> callback) = 0;
    virtual ~TimerInterface() = default;
};


class M4Lib : public QObject
{
    Q_OBJECT
public:

    enum class M4State {
        NONE           = 0,
        AWAIT          = 1,
        BIND           = 2,
        CALIBRATION    = 3,
        SETUP          = 4,
        RUN            = 5,
        SIM            = 6,
        FACTORY_CAL    = 7
    };

    /* Structure to save binding information. */
    struct RxBindInfo {
        enum class Type {
            NUL    = -1,
            SR12S  = 0,
            SR12E  = 1,
            SR24S  = 2,
            RX24   = 3,
            SR19P  = 4,
        };
        int mode = 0; // To store Type.
        int panId = 0;
        int nodeId = 0;
        int aNum = 0;
        int aBit = 0;
        int trNum = 0;
        int trBit = 0;
        int swNum = 0;
        int swBit = 0;
        int monitNum = 0;
        int monitBit = 0;
        int extraNum = 0;
        int extraBit = 0;
        int txAddr = 0;
        std::vector<uint8_t> achName {};
        std::vector<uint8_t> trName {};
        std::vector<uint8_t> swName {};
        std::vector<uint8_t> monitName {};
        std::vector<uint8_t> extraName {};
    };

    struct ControllerLocation {
        /**
         * Longitude of remote-controller
         */
        double longitude = 0.0;
        /**
         * Latitude of remote-controller
         */
        double latitude = 0.0;
        /**
         * Altitude of remote-controller
         */
        double altitude = 0.0;
        /**
         * The number of satellite has searched
         */
        int satelliteCount = 0;

        /**
         * Accuracy of remote-controller
         */
        float accuracy = 0.0f;

        /**
         * Speed of remote-controller
         */
        float speed = 0.0f;

        /**
         * Angle of remote-controller
         */
        float angle = 0.0f;
    };

    enum class SwitchId {
        OBSTACLE_AVOIDENCE,
    };

    enum class SwitchState {
        OFF,
        CENTER,
        ON
    };

    enum class ButtonId {
        POWER,
        CAMERA_SHUTTER,
        VIDEO_SHUTTER,
    };

    enum class ButtonState {
        NORMAL,
        PRESSED
    };

    void init();
    void deinit();

    void setPairCommandCallback(std::function<void()> callback);
    void setSwitchStateChangedCallback(std::function<void(SwitchId, SwitchState)> callback);
    void setButtonStateChangedCallback(std::function<void(ButtonId, ButtonState)> callback);
    void setRcActiveChangedCallback(std::function<void()> callback);
    void setCalibrationCompleteChangedCallback(std::function<void()> callback);
    void setCalibrationStateChangedCallback(std::function<void()> callback);
    void setRawChannelsChangedCallback(std::function<void()> callback);
    void setControllerLocationChangedCallback(std::function<void()> callback);
    void setM4StateChangedCallback(std::function<void()> callback);
    void setSaveSettingsCallback(std::function<void(const RxBindInfo& rxBindInfo)> callback);
    void setSettings(const RxBindInfo& rxBindInfo);

    void tryRead();

    M4State getM4State();

    bool getRcActive();
    void setRcActive(bool rcActive);

    bool getRcCalibrationComplete();

    void setVehicleConnected(bool vehicleConnected);

    std::vector<uint16_t> getRawChannels();

    const ControllerLocation& getControllerLocation();

    // TODO: Check if we really don't need this.
    //       If possible we don't want to leak this information.
    //bool getSoftReboot() { return _softReboot; }

    void resetBind();
    void enterBindMode(bool skipPairCommand = false);

    void checkVehicleReady();
    void tryStartCalibration();
    void tryStopCalibration();

    void softReboot();

    std::string m4StateStr();

    bool setPowerKey(int function);
    int calChannel(int index);

#if defined(__androidx86__)
    // These need to be ifdefd, otherwise we get linking errors.
    M4Lib(TimerInterface& timer);
    ~M4Lib();

private:
    void _bytesReady(QByteArray data);
    void _initSequence();
    void _stateManager();
    void _initAndCheckBinding();

    bool _write(QByteArray data, bool debug);
    void _tryEnterBindMode();
    bool _exitToAwait();
    bool _enterRun();
    bool _exitRun();
    bool _enterBind();
    bool _enterFactoryCalibration();
    bool _exitFactoryCalibration();
    bool _sendRecvBothCh();
    bool _exitBind();
    bool _startBind();
    bool _bind(int rxAddr);
    bool _setChannelSetting();
    bool _unbind();
    bool _queryBindState();
    bool _syncMixingDataDeleteAll();
    bool _syncMixingDataAdd();
    bool _sendTableDeviceLocalInfo(TableDeviceLocalInfo_t localInfo);
    bool _sendTableDeviceChannelInfo(TableDeviceChannelInfo_t channelInfo);
    bool _sendTableDeviceChannelNumInfo(ChannelNumType_t channelNumType);
    bool _sendRxResInfo();

    bool _sendPassthroughMessage(QByteArray message);

    bool _generateTableDeviceChannelNumInfo(TableDeviceChannelNumInfo_t* channelNumInfo, ChannelNumType_t channelNumType, int& num);
    bool _fillTableDeviceChannelNumMap       (TableDeviceChannelNumInfo_t *channelNumInfo, int num, std::vector<uint8_t> list);
    void _generateTableDeviceLocalInfo       (TableDeviceLocalInfo_t *localInfo);
    bool _generateTableDeviceChannelInfo     (TableDeviceChannelInfo_t *channelInfo);

    void _handleBindResponse                 ();
    void _handleQueryBindResponse            (QByteArray data);
    bool _handleNonTypePacket                (m4Packet& packet);
    void _handleRxBindInfo                   (m4Packet& packet);
    void _handleChannel                      (m4Packet& packet);
    bool _handleCommand                      (m4Packet& packet);
    void _switchChanged                      (m4Packet& packet);
    void _calibrationStateChanged            (m4Packet& packet);
    void _handleMixedChannelData             (m4Packet& packet);
    void _handleRawChannelData               (m4Packet& packet);
    void _handleControllerFeedback           (m4Packet& packet);
    void _handlePassThroughPacket            (m4Packet& packet);
    std::string _getRxBindInfoFeedbackName   ();

    static  int     _byteArrayToInt  (QByteArray data, int offset, bool isBigEndian = false);
    static  short   _byteArrayToShort(QByteArray data, int offset, bool isBigEndian = false);

    M4SerialComm* _commPort;

    TimerInterface& _timer;

    enum {
        STATE_NONE,
        STATE_ENTER_BIND_ERROR,
        STATE_EXIT_RUN,
        STATE_ENTER_BIND,
        STATE_START_BIND,
        STATE_UNBIND,
        STATE_BIND,
        STATE_QUERY_BIND,
        STATE_EXIT_BIND,
        STATE_RECV_BOTH_CH,
        STATE_SET_CHANNEL_SETTINGS,
        STATE_MIX_CHANNEL_DELETE,
        STATE_MIX_CHANNEL_ADD,
        STATE_SEND_RX_INFO,
        STATE_ENTER_RUN,
        STATE_RUNNING
    };

    std::function<void()>   _pairCommandCallback = nullptr;
    std::function<void(SwitchId, SwitchState)> _switchStateChangedCallback = nullptr;
    std::function<void(ButtonId, ButtonState)> _buttonStateChangedCallback = nullptr;
    std::function<void()> _rcActiveChangedCallback = nullptr;
    std::function<void()> _calibrationCompleteChangedCallback = nullptr;
    std::function<void()> _calibrationStateChangedCallback = nullptr;
    std::function<void()> _rawChannelsChangedCallback = nullptr;
    std::function<void()> _controllerLocationChangedCallback = nullptr;
    std::function<void()> _m4StateChangedCallback = nullptr;
    std::function<void(const RxBindInfo&)> _saveSettingsCallback = nullptr;

    int                     _state;
    int                     _responseTryCount;
    M4State                 _m4State;
    uint8_t                 _channelNumIndex;
    RxBindInfo              _rxBindInfoFeedback;
    int                     _currentChannelAdd;
    uint8_t                 _rxLocalIndex;
    uint8_t                 _rxchannelInfoIndex;
    bool                    _sendRxInfoEnd;
    bool                    _softReboot;
    bool                    _rcActive;
    uint16_t                _rawChannelsCalibration[CalibrationHwIndexMax];
    bool                    _rcCalibrationComplete;
    bool                    _vehicleConnected;
    bool                    _binding;
    std::vector<uint16_t>   _rawChannels;
    ControllerLocation      _controllerLocation;
#endif // defined(__androidx86__)
};

