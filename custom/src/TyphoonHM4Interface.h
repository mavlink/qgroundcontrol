/*!
 * @file
 *   @brief ST16 Controller
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#pragma once

#include "TyphoonHCommon.h"
#include "TyphoonHQuickInterface.h"

#include "m4lib/m4lib.h"

#include "Vehicle.h"

//-----------------------------------------------------------------------------
// Timer class for M4Lib
class TimerUsingQt : public QObject, public TimerInterface {
    Q_OBJECT
signals:
    void started(int time_ms);
    void stopped();

public:
    TimerUsingQt(QObject* parent = NULL) {
        Q_UNUSED(parent);
        _timer.setSingleShot(true);
        connect(&_timer, &QTimer::timeout, this, &TimerUsingQt::_timeout);
        connect(this, &TimerUsingQt::started, this, &TimerUsingQt::_started);
        connect(this, &TimerUsingQt::stopped, this, &TimerUsingQt::_stopped);
    }

    void start(int time_ms) final {
        // The QT Timer needs to be called from the main thread.
        emit started(time_ms);
    }

    void stop() final {
        emit stopped();
    }

    void setCallback(std::function<void()> callback) final {
        _callback = callback;
    }

    virtual ~TimerUsingQt()
    {
        qCDebug(YuneecLogVerbose) << "timer destructed";
        _callback = nullptr;
    }

private slots:
    void _started(int time_ms) {
        _timer.start(time_ms);
    }

    void _stopped() {
        _timer.stop();
    }

private:
    void _timeout()
    {
        if (_callback) {
            _callback();
        } else {
            qCDebug(YuneecLogVerbose) << "callback not set";
            abort();
        }
    }

    QTimer _timer;
    std::function<void()> _callback = nullptr;
};

//-----------------------------------------------------------------------------
// Sleeper class for M4Lib
class SleeperUsingQt : public SleeperInterface {
public:
    SleeperUsingQt() {
    }

    void msleep(int duration_ms) final {
        QThread::msleep(duration_ms);
    }
};

//-----------------------------------------------------------------------------
// Interface to everything St16 specific including the M4 microprocessor.
class TyphoonHM4Interface : public QThread
{
    Q_OBJECT
public:
    TyphoonHM4Interface(QObject* parent = NULL);
    ~TyphoonHM4Interface();

    void    init                    (bool skipConnections = false);
    void    enterBindMode           (bool skipPairCommand = false);
    void    initM4                  ();
    QString m4StateStr              ();
    void    resetBind               ();
    bool    rcActive                ();
    bool    rcCalibrationComplete   ();
    void    startCalibration        ();
    void    stopCalibration         ();


    QList<uint16_t>     rawChannels     ();
    int                 calChannel      (int index);

    TyphoonHQuickInterface::M4State     m4State             ();

    const M4Lib::ControllerLocation&           controllerLocation  ();

    static TyphoonHM4Interface* pTyphoonHandler;

    //-- From QThread
    void        run                             ();

public slots:
    void    softReboot                          ();

private slots:
    void    _vehicleAdded                       (Vehicle* vehicle);
    void    _vehicleRemoved                     (Vehicle* vehicle);
    void    _vehicleReady                       (bool ready);
    void    _mavlinkMessageReceived             (const mavlink_message_t& message);
    void    _rcTimeout                          ();
    void    _rcActiveChanged                    ();
    void    _saveSettings                       (const M4Lib::RxBindInfo& rxBindInfo);
    void    _sendMavlinkBindCommand             ();

signals:
    void    sendMavlinkBindCommand              ();
    void    m4StateChanged                      ();
    void    buttonStateChanged                  (M4Lib::ButtonId buttonId, M4Lib::ButtonState buttonState);
    void    switchStateChanged                  (M4Lib::SwitchId switchId, M4Lib::SwitchState switchState);
    void    destroyed                           ();
    void    controllerLocationChanged           ();
    void    armedChanged                        (bool armed);
    void    rawChannelsChanged                  ();
    void    calibrationCompleteChanged          ();
    void    calibrationStateChanged             ();
    void    rcActiveChanged                     ();
    void    saveSettings                        (const M4Lib::RxBindInfo& rxBindInfo);
    void    distanceSensor                      (int minDist, int maxDist, int curDist);


    //-- WIFI
    void    newWifiSSID                         (QString ssid, int rssi);
    void    newWifiRSSI                         ();
    void    scanComplete                        ();
    void    authenticationError                 ();
    void    wifiConnected                       ();
    void    wifiDisconnected                    ();
    void    batteryUpdate                       ();

private:

#if defined(__androidx86__)
    TimerUsingQt            _m4LibTimer;
    SleeperUsingQt          _m4LibSleeper;
    M4Lib*                  _m4Lib;
#endif
    Vehicle*                _vehicle;
    uint32_t                _rcTime;
    bool                    _threadShouldStop;
    QTimer                  _rcTimer;
};

