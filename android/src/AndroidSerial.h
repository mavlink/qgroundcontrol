#pragma once

#include <QtCore/QString>
#include <QtCore/QByteArray>
#include <QtCore/QLoggingCategory>

#include <qserialport.h>
#include <qserialportinfo.h>

#include <jni.h>

Q_DECLARE_LOGGING_CATEGORY(AndroidSerialLog);

class QSerialPortPrivate;

namespace AndroidSerial
{
    enum DataBits {
        Data5 = 5,
        Data6 = 6,
        Data7 = 7,
        Data8 = 8
    };

    enum Parity {
        NoParity = 0,
        OddParity,
        EvenParity,
        MarkParity,
        SpaceParity
    };

    enum StopBits {
        OneStop = 1,
        OneAndHalfStop = 3,
        TwoStop = 2
    };

    enum ControlLine {
        RtsControlLine = 0,
        CtsControlLine,
        DtrControlLine,
        DsrControlLine,
        CdControlLine,
        RiControlLine
    };

    enum FlowControl {
        NoFlowControl = 0,
        RtsCtsFlowControl,
        DtrDsrFlowControl,
        XonXoffFlowControl,
        XonXoffInlineFlowControl
    };

    constexpr char CHAR_XON = 17;
    constexpr char CHAR_XOFF = 19;

    constexpr const char *kJniUsbSerialManagerClassName = "org/mavlink/qgroundcontrol/QGCUsbSerialManager";

    jclass getSerialManagerClass();
    void setNativeMethods();
    void jniDeviceHasDisconnected(JNIEnv *env, jobject obj, jlong classPtr);
    void jniDeviceNewData(JNIEnv *env, jobject obj, jlong classPtr, jbyteArray data);
    void jniDeviceException(JNIEnv *env, jobject obj, jlong classPtr, jstring message);
    QList<QSerialPortInfo> availableDevices();
    int getDeviceId(const QString &portName);
    int getDeviceHandle(int deviceId);
    int open(const QString &portName, QSerialPortPrivate *classPtr);
    bool close(int deviceId);
    bool isOpen(const QString &portName);
    QByteArray read(int deviceId, int length, int timeout);
    int write(int deviceId, const char *data, int length, int timeout, bool async);
    bool setParameters(int deviceId, int baudRate, int dataBits, int stopBits, int parity);
    bool getCarrierDetect(int deviceId);
    bool getClearToSend(int deviceId);
    bool getDataSetReady(int deviceId);
    bool getDataTerminalReady(int deviceId);
    bool setDataTerminalReady(int deviceId, bool set);
    bool getRingIndicator(int deviceId);
    bool getRequestToSend(int deviceId);
    bool setRequestToSend(int deviceId, bool set);
    QSerialPort::PinoutSignals getControlLines(int deviceId);
    int getFlowControl(int deviceId);
    bool setFlowControl(int deviceId, int flowControl);
    bool purgeBuffers(int deviceId, bool input, bool output);
    bool setBreak(int deviceId, bool set);
    bool startReadThread(int deviceId);
    bool stopReadThread(int deviceId);
    bool readThreadRunning(int deviceId);
} // namespace AndroidSerial
