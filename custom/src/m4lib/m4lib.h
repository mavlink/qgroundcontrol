#pragma once

#include "TyphoonHCommon.h"
#include "m4serial.h"
#include "m4util.h"

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

    void init();
    void deinit();

    void setPairCommandCallback(std::function<void()> callback);

    void setSettings(const RxBindInfo& rxBindInfo);

    void tryRead();

    M4State getM4State() { return _m4State; }

    bool getRcActive() { return _rcActive; }
    void setRcActive(bool rcActive) { _rcActive = rcActive; }

    bool getRcCalibrationComplete() { return _rcCalibrationComplete; }

    void setVehicleConnected(bool vehicleConnected) { _vehicleConnected = vehicleConnected; }

    std::vector<uint16_t> getRawChannels() { return _rawChannels; }

    const ControllerLocation& getControllerLocation() { return _controllerLocation; }

    bool getSoftReboot() { return _softReboot; }
    void setSoftReboot(bool softReboot) { _softReboot = softReboot; }

    void resetBind();
    void enterBindMode(bool skipPairCommand = false);

    void checkVehicleReady();
    void tryStartCalibration();
    void tryStopCalibration();

    void softReboot();

    std::string m4StateStr();

    bool setPowerKey(int function);
    int calChannel(int index);

    M4Lib(QObject* parent = NULL);
    ~M4Lib();

signals:
    void rcActiveChanged();
    void switchStateChanged                  (int swId, int oldState, int newState);
    void calibrationCompleteChanged          ();
    void calibrationStateChanged             ();
    void rawChannelsChanged                  ();
    void controllerLocationChanged           ();
    void m4StateChanged                      ();
    void saveSettings                        (const RxBindInfo& rxBindInfo);

private slots:
    void _bytesReady(QByteArray data);
    void _initSequence();
    void _stateManager();
    void _initAndCheckBinding                ();

private:
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
    bool _sendRxResInfo                      ();

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

    int                     _state;
    int                     _responseTryCount;
    M4State                 _m4State;
    uint8_t                 _channelNumIndex;
    RxBindInfo              _rxBindInfoFeedback;
    QTimer                  _timer;
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
};
