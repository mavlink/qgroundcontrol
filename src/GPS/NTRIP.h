/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <atomic>

#include <QtCore/QElapsedTimer>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtNetwork/QTcpSocket>
#include <QtPositioning/QGeoCoordinate>

Q_DECLARE_LOGGING_CATEGORY(NTRIPLog)

class RTCMMavlink;
class NTRIPSettings;
class Vehicle;
class MultiVehicleManager;

#define RTCM3_PREAMBLE 0xD3

class RTCMParser
{
public:
    RTCMParser();
    void reset();
    bool addByte(uint8_t byte);
    uint8_t* message() { return _buffer; }
    uint16_t messageLength() { return _messageLength; }
    uint16_t messageId();
    const uint8_t* crcBytes() const { return _crcBytes; }
    int crcSize() const { return 3; }    

private:
    enum State {
        WaitingForPreamble,
        ReadingLength,
        ReadingMessage,
        ReadingCRC
    };
    
    State _state;
    uint8_t _buffer[1024];
    uint16_t _messageLength;
    uint16_t _bytesRead;
    uint16_t _lengthBytesRead;
    uint8_t _lengthBytes[2];
    uint16_t _crcBytesRead;
    uint8_t _crcBytes[3];
};

class NTRIPTCPLink : public QObject
{
    Q_OBJECT

public:
    NTRIPTCPLink(const QString& hostAddress,
                 int port,
                 const QString& username,
                 const QString& password,
                 const QString& mountpoint,
                 const QString& whitelist,
                 bool useSpartn,
                 QObject* parent = nullptr);

    Q_INVOKABLE void debugFetchSourceTable();

    ~NTRIPTCPLink();
    
public slots:
    void start();        
    void requestStop();
    void sendNMEA(const QByteArray& sentence);

signals:
    void finished();
    void error(const QString& errorMsg);
    void RTCMDataUpdate(const QByteArray& message);
    // Called when SPARTN corrections are received from the NTRIPTCPLink
    // These corrections are forwarded to the connected vehicle (PX4/ArduPilot)
    // using the same path as RTCM corrections via _rtcmDataReceived().    
    void SPARTNDataUpdate(const QByteArray& message);
    void connected();


private slots:
    void _readBytes();

private:
    enum class NTRIPState {
        uninitialised,
        waiting_for_http_response,
        waiting_for_spartn_data,
        waiting_for_rtcm_header,
        accumulating_rtcm_packet,
    };
    
    void _hardwareConnect();
    void _parse(const QByteArray& buffer);
    void _handleSpartnData(const QByteArray& data);    

    QTcpSocket* _socket = nullptr;
    
    QString _hostAddress;
    int _port;
    QString _username;
    QString _password;
    QString _mountpoint;
    bool _useSpartn = false;    
    QVector<int> _whitelist;

    RTCMParser* _rtcmParser = nullptr;
    NTRIPState _state;
    
    std::atomic<bool> _stopping{false};
    QMetaObject::Connection _readyReadConn;
    
    // Small buffer to strip a response header only once if needed
    QByteArray _spartnBuf;
    bool _spartnNeedHeaderStrip = true;        

};

class NTRIPManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString ntripStatus READ ntripStatus NOTIFY ntripStatusChanged)
    Q_PROPERTY(CasterStatus casterStatus READ casterStatus NOTIFY casterStatusChanged)
    Q_PROPERTY(QString ggaSource READ ggaSource NOTIFY ggaSourceChanged)

public:
    enum class CasterStatus { Connected, NoLocationError, OtherError };

    NTRIPManager(QObject* parent = nullptr);
    ~NTRIPManager();

    static NTRIPManager* instance();

    QString ntripStatus() const { return _ntripStatus; }
    CasterStatus casterStatus() const { return _casterStatus; }
    QString ggaSource() const { return _ggaSource; }

    void startNTRIP();
    void stopNTRIP();

signals:
    void ntripStatusChanged();
    void casterStatusChanged(CasterStatus status);
    void ggaSourceChanged();

public slots:
    void _tcpError(const QString& errorMsg);
    void _rtcmDataReceived(const QByteArray& data);

private:
    void _checkSettings();
    void _sendGGA();
    void _onCasterDisconnected(const QString& reason);

    QTimer* _ggaTimer = nullptr;

    QString _ntripStatus;
    QString _ggaSource;
    QMetaObject::Connection _ntripEnableConn;
    
    NTRIPTCPLink* _tcpLink = nullptr;
    QThread* _tcpThread = nullptr;
    RTCMMavlink* _rtcmMavlink = nullptr;
    QTimer* _settingsCheckTimer = nullptr;
    bool _startStopBusy = false;
    bool _forcedOffOnce = false;
    bool _useSpartn = false;    
    
    QElapsedTimer _startupTimer;
    bool _startupSuppress = true;
    int  _startupStableTicks = 0;    
    
    CasterStatus _casterStatus = CasterStatus::OtherError;

    static NTRIPManager* _instance;
};