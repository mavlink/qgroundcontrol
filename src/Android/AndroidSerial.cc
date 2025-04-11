#include "AndroidSerial.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QJniEnvironment>
#include <QtCore/QJniObject>

#include <qserialport_p.h>
#include <qserialportinfo_p.h>

QGC_LOGGING_CATEGORY(AndroidSerialLog, "qgc.android.androidserial");

// TODO: Save Method Lookups

namespace AndroidSerial
{

jclass getSerialManagerClass()
{
    static jclass javaClass = nullptr;

    if (!javaClass) {
        QJniEnvironment env;
        if (!env.isValid()) {
            qCWarning(AndroidSerialLog) << "Invalid QJniEnvironment";
            return nullptr;
        }

        if (!QJniObject::isClassAvailable(kJniUsbSerialManagerClassName)) {
            qCWarning(AndroidSerialLog) << "Class Not Available:" << kJniUsbSerialManagerClassName;
            return nullptr;
        }

        javaClass = env.findClass(kJniUsbSerialManagerClassName);
        if (!javaClass) {
            qCWarning(AndroidSerialLog) << "Class Not Found:" << kJniUsbSerialManagerClassName;
            return nullptr;
        }

        (void) env.checkAndClearExceptions();
    }

    return javaClass;
}

void setNativeMethods()
{
    qCDebug(AndroidSerialLog) << "Registering Native Functions";

    const JNINativeMethod javaMethods[] {
        {"nativeDeviceHasDisconnected", "(J)V",                     reinterpret_cast<void*>(jniDeviceHasDisconnected)},
        {"nativeDeviceNewData",         "(J[B)V",                   reinterpret_cast<void*>(jniDeviceNewData)},
        {"nativeDeviceException",       "(JLjava/lang/String;)V",   reinterpret_cast<void*>(jniDeviceException)},
    };

    QJniEnvironment jniEnv;
    const jclass javaClass = getSerialManagerClass();
    if (!javaClass) {
        qCWarning(AndroidSerialLog) << "Couldn't find class for RegisterNatives:" << kJniUsbSerialManagerClassName;
        (void) jniEnv.checkAndClearExceptions();
        return;
    }

    const jint regResult = jniEnv->RegisterNatives(javaClass, javaMethods, std::size(javaMethods));
    if (regResult != JNI_OK) {
        qCWarning(AndroidSerialLog) << "Error registering native methods:" << regResult;
        (void) jniEnv.checkAndClearExceptions();
        return;
    }

    qCDebug(AndroidSerialLog) << "Native Functions Registered Successfully";

    (void) jniEnv.checkAndClearExceptions();
}

void jniDeviceHasDisconnected(JNIEnv *env, jobject obj, jlong classPtr)
{
    Q_UNUSED(env); Q_UNUSED(obj);

    if (classPtr == 0) {
        qCWarning(AndroidSerialLog) << "nativeDeviceHasDisconnected called with classPtr=0";
        return;
    }

    QSerialPortPrivate* const serialPortPrivate = reinterpret_cast<QSerialPortPrivate*>(classPtr);
    if (!serialPortPrivate) {
        qCWarning(AndroidSerialLog) << "serialPortPrivate is null in nativeDeviceHasDisconnected";
        return;
    }

    QSerialPort* const serialPort = qobject_cast<QSerialPort*>(serialPortPrivate->q_ptr);
    if (!serialPort) {
        qCWarning(AndroidSerialLog) << "serialPort is null in nativeDeviceHasDisconnected";
        return;
    }

    qCDebug(AndroidSerialLog) << "Device disconnected:" << serialPort->portName();

    if (serialPort->isOpen()) {
        serialPort->close();
        qCDebug(AndroidSerialLog) << "Serial port closed in nativeDeviceHasDisconnected";
    } else {
        qCWarning(AndroidSerialLog) << "Serial port was already closed in nativeDeviceHasDisconnected";
    }
}

void jniDeviceNewData(JNIEnv *env, jobject obj, jlong classPtr, jbyteArray data)
{
    Q_UNUSED(obj);

    if (classPtr == 0) {
        qCWarning(AndroidSerialLog) << "nativeDeviceNewData called with classPtr=0";
        return;
    }

    if (!data) {
        qCWarning(AndroidSerialLog) << "nativeDeviceNewData called with null data";
        return;
    }

    const jsize len = env->GetArrayLength(data);
    if (len <= 0) {
        qCWarning(AndroidSerialLog) << "nativeDeviceNewData received empty data array";
        return;
    }

    jbyte* const bytes = env->GetByteArrayElements(data, nullptr);
    if (!bytes) {
        qCWarning(AndroidSerialLog) << "Failed to get byte array elements";
        return;
    }

    const QByteArray byteArray(reinterpret_cast<char*>(bytes), len);
    env->ReleaseByteArrayElements(data, bytes, JNI_ABORT);

    QSerialPortPrivate* const serialPortPrivate = reinterpret_cast<QSerialPortPrivate*>(classPtr);
    if (!serialPortPrivate) {
        qCWarning(AndroidSerialLog) << "serialPortPrivate is null";
        return;
    }

    serialPortPrivate->newDataArrived(byteArray.constData(), byteArray.size());

    if (QJniEnvironment::checkAndClearExceptions(env)) {
        qCWarning(AndroidSerialLog) << "Exception occurred in nativeDeviceNewData";
    }
}

void jniDeviceException(JNIEnv *env, jobject obj, jlong classPtr, jstring message)
{
    Q_UNUSED(obj);

    if (classPtr == 0) {
        qCWarning(AndroidSerialLog) << "nativeDeviceException called with classPtr=0";
        return;
    }

    if (!message) {
        qCWarning(AndroidSerialLog) << "nativeDeviceException called with null message";
        return;
    }

    const char *const rawMessage = env->GetStringUTFChars(message, nullptr);
    if (!rawMessage) {
        qCWarning(AndroidSerialLog) << "Failed to get UTF chars from jstring";
        return;
    }

    const QString exceptionMessage = QString::fromUtf8(rawMessage);
    env->ReleaseStringUTFChars(message, rawMessage);

    QSerialPortPrivate* const serialPortPrivate = reinterpret_cast<QSerialPortPrivate*>(classPtr);
    if (!serialPortPrivate) {
        qCWarning(AndroidSerialLog) << "serialPortPrivate is null";
        return;
    }

    serialPortPrivate->exceptionArrived(exceptionMessage);
    qCWarning(AndroidSerialLog) << "Exception from Java:" << exceptionMessage;

    if (QJniEnvironment::checkAndClearExceptions(env)) {
        qCWarning(AndroidSerialLog) << "Exception occurred in nativeDeviceException";
    }
}

QList<QSerialPortInfo> availableDevices()
{
    QList<QSerialPortInfo> serialPortInfoList;

    const jclass javaClass = getSerialManagerClass();
    if (!javaClass) {
        qCWarning(AndroidSerialLog) << "getSerialManagerClass returned null in availableDevices";
        return serialPortInfoList;
    }

    QJniEnvironment env;
    if (!env.isValid()) {
        qCWarning(AndroidSerialLog) << "Invalid QJniEnvironment in availableDevices";
        return serialPortInfoList;
    }

    const jmethodID methodId = env->GetStaticMethodID(javaClass, "availableDevicesInfo", "()[Ljava/lang/String;");
    if (!methodId) {
        qCWarning(AndroidSerialLog) << "Method Not Found: availableDevicesInfo";
        env.checkAndClearExceptions();
        return serialPortInfoList;
    }

    jobjectArray objArray = static_cast<jobjectArray>(env->CallStaticObjectMethod(javaClass, methodId));
    if (!objArray) {
        qCWarning(AndroidSerialLog) << "availableDevicesInfo returned null";
        (void) env.checkAndClearExceptions();
        return serialPortInfoList;
    }

    if (env.checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception occurred while calling availableDevicesInfo";
        env->DeleteLocalRef(objArray);
        return serialPortInfoList;
    }

    const jsize count = env->GetArrayLength(objArray);
    for (jsize i = 0; i < count; ++i) {
        jstring jstr = static_cast<jstring>(env->GetObjectArrayElement(objArray, i));
        if (!jstr) {
            qCWarning(AndroidSerialLog) << "Null string at index" << i;
            continue;
        }

        const char *const rawString = env->GetStringUTFChars(jstr, nullptr);
        if (!rawString) {
            qCWarning(AndroidSerialLog) << "Failed to get UTF chars from jstring at index" << i;
            env->DeleteLocalRef(jstr);
            continue;
        }

        const QStringList strList = QString::fromUtf8(rawString).split(':');
        env->ReleaseStringUTFChars(jstr, rawString);
        env->DeleteLocalRef(jstr);

        if (strList.size() < 6) {
            qCWarning(AndroidSerialLog) << "Invalid device info at index" << i << ":" << strList;
            continue;
        }

        bool pidOK, vidOK;
        QSerialPortInfoPrivate info;
        info.portName = QSerialPortInfoPrivate::portNameFromSystemLocation(strList[0]);
        info.device = strList[0];
        info.description = strList[1];
        info.manufacturer = strList[2];
        info.serialNumber = strList[3];
        info.productIdentifier = strList[4].toInt(&pidOK);
        info.hasProductIdentifier = (pidOK && (info.productIdentifier != INVALID_DEVICE_ID));
        info.vendorIdentifier = strList[5].toInt(&vidOK);
        info.hasVendorIdentifier = (vidOK && (info.vendorIdentifier != INVALID_DEVICE_ID));

        serialPortInfoList.append(info);
    }

    env->DeleteLocalRef(objArray);
    (void) env.checkAndClearExceptions();

    return serialPortInfoList;
}

int getDeviceId(const QString &portName)
{
    const QJniObject name = QJniObject::fromString(portName);
    if (!name.isValid()) {
        qCWarning(AndroidSerialLog) << "Invalid QJniObject for portName in getDeviceId";
        return -1;
    }

    const jclass javaClass = getSerialManagerClass();
    if (!javaClass) {
        qCWarning(AndroidSerialLog) << "getSerialManagerClass returned null in getDeviceId";
        return -1;
    }

    QJniEnvironment env;
    if (!env.isValid()) {
        qCWarning(AndroidSerialLog) << "Invalid QJniEnvironment in getDeviceId";
        return -1;
    }

    const jmethodID methodId = env->GetStaticMethodID(javaClass, "getDeviceId", "(Ljava/lang/String;)I");
    if (!methodId) {
        qCWarning(AndroidSerialLog) << "Method Not Found: getDeviceId";
        env.checkAndClearExceptions();
        return -1;
    }

    const jint result = env->CallStaticIntMethod(javaClass, methodId, name.object<jstring>());
    if (env.checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception occurred while calling getDeviceId";
        return -1;
    }

    return static_cast<int>(result);
}

int getDeviceHandle(int deviceId)
{
    QJniEnvironment env;
    if (!env.isValid()) {
        qCWarning(AndroidSerialLog) << "Invalid QJniEnvironment in getDeviceHandle";
        return -1;
    }

    const jclass javaClass = getSerialManagerClass();
    if (!javaClass) {
        qCWarning(AndroidSerialLog) << "getSerialManagerClass returned null in getDeviceHandle";
        return -1;
    }

    const jmethodID methodId = env->GetStaticMethodID(javaClass, "getDeviceHandle", "(I)I");
    if (!methodId) {
        qCWarning(AndroidSerialLog) << "Method Not Found: getDeviceHandle";
        env.checkAndClearExceptions();
        return -1;
    }

    const jint result = env->CallStaticIntMethod(javaClass, methodId, static_cast<jint>(deviceId));
    if (env.checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception occurred while calling getDeviceHandle";
        return -1;
    }

    return static_cast<int>(result);
}

int open(const QString &portName, QSerialPortPrivate *classPtr)
{
    if (!classPtr) {
        qCWarning(AndroidSerialLog) << "open called with null serialPort";
        return -1;
    }

    const QJniObject name = QJniObject::fromString(portName);
    if (!name.isValid()) {
        qCWarning(AndroidSerialLog) << "Invalid QJniObject for portName in open";
        return -1;
    }

    const jclass javaClass = getSerialManagerClass();
    if (!javaClass) {
        qCWarning(AndroidSerialLog) << "getSerialManagerClass returned null in open";
        return -1;
    }

    QJniEnvironment env;
    if (!env.isValid()) {
        qCWarning(AndroidSerialLog) << "Invalid QJniEnvironment in open";
        return -1;
    }

    const jmethodID methodId = env->GetStaticMethodID(javaClass, "open", "(Ljava/lang/String;J)I");
    if (!methodId) {
        qCWarning(AndroidSerialLog) << "Method Not Found: open";
        env.checkAndClearExceptions();
        return -1;
    }

    const jint deviceId = env->CallStaticIntMethod(javaClass, methodId, name.object<jstring>(), reinterpret_cast<jlong>(classPtr));
    if (env.checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception occurred while calling open";
        return -1;
    }

    return static_cast<int>(deviceId);
}

bool close(int deviceId)
{
    QJniEnvironment env;
    if (!env.isValid()) {
        qCWarning(AndroidSerialLog) << "Invalid QJniEnvironment in close";
        return false;
    }

    const jclass javaClass = getSerialManagerClass();
    if (!javaClass) {
        qCWarning(AndroidSerialLog) << "getSerialManagerClass returned null in close";
        return false;
    }

    const jmethodID methodId = env->GetStaticMethodID(javaClass, "close", "(I)Z");
    if (!methodId) {
        qCWarning(AndroidSerialLog) << "Method Not Found: close";
        env.checkAndClearExceptions();
        return false;
    }

    const jboolean result = env->CallStaticBooleanMethod(javaClass, methodId, static_cast<jint>(deviceId));
    if (env.checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception occurred while calling close";
        return false;
    }

    return (result == JNI_TRUE);
}

bool isOpen(const QString &portName)
{
    const QJniObject name = QJniObject::fromString(portName);
    if (!name.isValid()) {
        qCWarning(AndroidSerialLog) << "Invalid QJniObject for portName in isOpen";
        return false;
    }

    const jclass javaClass = getSerialManagerClass();
    if (!javaClass) {
        qCWarning(AndroidSerialLog) << "getSerialManagerClass returned null in isOpen";
        return false;
    }

    QJniEnvironment env;
    if (!env.isValid()) {
        qCWarning(AndroidSerialLog) << "Invalid QJniEnvironment in isOpen";
        return false;
    }

    const jmethodID methodId = env->GetStaticMethodID(javaClass, "isDeviceNameOpen", "(Ljava/lang/String;)Z");
    if (!methodId) {
        qCWarning(AndroidSerialLog) << "Method Not Found: isDeviceNameOpen";
        env.checkAndClearExceptions();
        return false;
    }

    const jboolean result = env->CallStaticBooleanMethod(javaClass, methodId, name.object<jstring>());
    if (env.checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception occurred while calling isDeviceNameOpen";
        return false;
    }

    return (result == JNI_TRUE);
}

QByteArray read(int deviceId, int length, int timeout)
{
    QJniEnvironment env;
    if (!env.isValid()) {
        qCWarning(AndroidSerialLog) << "Invalid QJniEnvironment in read";
        return QByteArray();
    }

    const jclass javaClass = getSerialManagerClass();
    if (!javaClass) {
        qCWarning(AndroidSerialLog) << "getSerialManagerClass returned null in read";
        return QByteArray();
    }

    const jmethodID methodId = env->GetStaticMethodID(javaClass, "read", "(III)[B");
    if (!methodId) {
        qCWarning(AndroidSerialLog) << "Method Not Found: read";
        env.checkAndClearExceptions();
        return QByteArray();
    }

    const jbyteArray jarray = static_cast<jbyteArray>(env->CallStaticObjectMethod(javaClass, methodId,
                                                                                  static_cast<jint>(deviceId),
                                                                                  static_cast<jint>(length),
                                                                                  static_cast<jint>(timeout)));

    if (!jarray) {
        qCWarning(AndroidSerialLog) << "read method returned null";
        (void) env.checkAndClearExceptions();
        return QByteArray();
    }

    if (env.checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception occurred while calling read";
        env->DeleteLocalRef(jarray);
        return QByteArray();
    }

    const jsize len = env->GetArrayLength(jarray);
    jbyte *const bytes = env->GetByteArrayElements(jarray, nullptr);
    if (!bytes) {
        qCWarning(AndroidSerialLog) << "Failed to get byte array elements in read";
        env->DeleteLocalRef(jarray);
        return QByteArray();
    }

    const QByteArray data(reinterpret_cast<char*>(bytes), len);
    env->ReleaseByteArrayElements(jarray, bytes, JNI_ABORT);
    env->DeleteLocalRef(jarray);

    return data;
}

int write(int deviceId, const char *data, int length, int timeout, bool async)
{
    if (!data || length <= 0) {
        qCWarning(AndroidSerialLog) << "Invalid data or length in write";
        return -1;
    }

    QJniEnvironment env;
    if (!env.isValid()) {
        qCWarning(AndroidSerialLog) << "Invalid QJniEnvironment in write";
        return -1;
    }

    const jclass javaClass = getSerialManagerClass();
    if (!javaClass) {
        qCWarning(AndroidSerialLog) << "getSerialManagerClass returned null in write";
        return -1;
    }

    const jbyteArray jarray = env->NewByteArray(static_cast<jsize>(length));
    if (!jarray) {
        qCWarning(AndroidSerialLog) << "Failed to create jbyteArray in write";
        return -1;
    }

    env->SetByteArrayRegion(jarray, 0, static_cast<jsize>(length), reinterpret_cast<const jbyte*>(data));
    if (env.checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception occurred while setting byte array region in write";
        env->DeleteLocalRef(jarray);
        return -1;
    }

    jint result;
    if (async) {
        const jmethodID methodId = env->GetStaticMethodID(javaClass, "writeAsync", "(I[BI)I");
        if (!methodId) {
            qCWarning(AndroidSerialLog) << "Method Not Found: writeAsync";
            env.checkAndClearExceptions();
            env->DeleteLocalRef(jarray);
            return -1;
        }

        result = env->CallStaticIntMethod(javaClass, methodId,
                                          static_cast<jint>(deviceId),
                                          jarray,
                                          static_cast<jint>(timeout));
    } else {
        const jmethodID methodId = env->GetStaticMethodID(javaClass, "write", "(I[BII)I");
        if (!methodId) {
            qCWarning(AndroidSerialLog) << "Method Not Found: write";
            env.checkAndClearExceptions();
            env->DeleteLocalRef(jarray);
            return -1;
        }

        result = env->CallStaticIntMethod(javaClass, methodId,
                                          static_cast<jint>(deviceId),
                                          jarray,
                                          static_cast<jint>(length),
                                          static_cast<jint>(timeout));
    }

    if (env.checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception occurred while calling write/writeAsync";
        env->DeleteLocalRef(jarray);
        return -1;
    }

    env->DeleteLocalRef(jarray);

    return static_cast<int>(result);
}

bool setParameters(int deviceId, int baudRate, int dataBits, int stopBits, int parity)
{
    QJniEnvironment env;
    if (!env.isValid()) {
        qCWarning(AndroidSerialLog) << "Invalid QJniEnvironment in setParameters";
        return false;
    }

    const jclass javaClass = getSerialManagerClass();
    if (!javaClass) {
        qCWarning(AndroidSerialLog) << "getSerialManagerClass returned null in setParameters";
        return false;
    }

    const jmethodID methodId = env->GetStaticMethodID(javaClass, "setParameters", "(IIIII)Z");
    if (!methodId) {
        qCWarning(AndroidSerialLog) << "Method Not Found: setParameters";
        env.checkAndClearExceptions();
        return false;
    }

    const jboolean result = env->CallStaticBooleanMethod(javaClass, methodId,
                                                         static_cast<jint>(deviceId),
                                                         static_cast<jint>(baudRate),
                                                         static_cast<jint>(dataBits),
                                                         static_cast<jint>(stopBits),
                                                         static_cast<jint>(parity));

    if (env.checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception occurred while calling setParameters";
        return false;
    }

    return (result == JNI_TRUE);
}

bool getCarrierDetect(int deviceId)
{
    QJniEnvironment env;
    if (!env.isValid()) {
        qCWarning(AndroidSerialLog) << "Invalid QJniEnvironment in getCarrierDetect";
        return false;
    }

    const jclass javaClass = getSerialManagerClass();
    if (!javaClass) {
        qCWarning(AndroidSerialLog) << "getSerialManagerClass returned null in getCarrierDetect";
        return false;
    }

    const jmethodID methodId = env->GetStaticMethodID(javaClass, "getCarrierDetect", "(I)Z");
    if (!methodId) {
        qCWarning(AndroidSerialLog) << "Method Not Found: getCarrierDetect";
        env.checkAndClearExceptions();
        return false;
    }

    const jboolean result = env->CallStaticBooleanMethod(javaClass, methodId, static_cast<jint>(deviceId));
    if (env.checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception occurred while calling getCarrierDetect";
        return false;
    }

    return (result == JNI_TRUE);
}

bool getClearToSend(int deviceId)
{
    QJniEnvironment env;
    if (!env.isValid()) {
        qCWarning(AndroidSerialLog) << "Invalid QJniEnvironment in getClearToSend";
        return false;
    }

    const jclass javaClass = getSerialManagerClass();
    if (!javaClass) {
        qCWarning(AndroidSerialLog) << "getSerialManagerClass returned null in getClearToSend";
        return false;
    }

    const jmethodID methodId = env->GetStaticMethodID(javaClass, "getClearToSend", "(I)Z");
    if (!methodId) {
        qCWarning(AndroidSerialLog) << "Method Not Found: getClearToSend";
        env.checkAndClearExceptions();
        return false;
    }

    const jboolean result = env->CallStaticBooleanMethod(javaClass, methodId, static_cast<jint>(deviceId));
    if (env.checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception occurred while calling getClearToSend";
        return false;
    }

    return (result == JNI_TRUE);
}

bool getDataSetReady(int deviceId)
{
    QJniEnvironment env;
    if (!env.isValid()) {
        qCWarning(AndroidSerialLog) << "Invalid QJniEnvironment in getDataSetReady";
        return false;
    }

    const jclass javaClass = getSerialManagerClass();
    if (!javaClass) {
        qCWarning(AndroidSerialLog) << "getSerialManagerClass returned null in getDataSetReady";
        return false;
    }

    const jmethodID methodId = env->GetStaticMethodID(javaClass, "getDataSetReady", "(I)Z");
    if (!methodId) {
        qCWarning(AndroidSerialLog) << "Method Not Found: getDataSetReady";
        env.checkAndClearExceptions();
        return false;
    }

    const jboolean result = env->CallStaticBooleanMethod(javaClass, methodId, static_cast<jint>(deviceId));
    if (env.checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception occurred while calling getDataSetReady";
        return false;
    }

    return (result == JNI_TRUE);
}

bool getDataTerminalReady(int deviceId)
{
    QJniEnvironment env;
    if (!env.isValid()) {
        qCWarning(AndroidSerialLog) << "Invalid QJniEnvironment in getDataTerminalReady";
        return false;
    }

    const jclass javaClass = getSerialManagerClass();
    if (!javaClass) {
        qCWarning(AndroidSerialLog) << "getSerialManagerClass returned null in getDataTerminalReady";
        return false;
    }

    const jmethodID methodId = env->GetStaticMethodID(javaClass, "getDataTerminalReady", "(I)Z");
    if (!methodId) {
        qCWarning(AndroidSerialLog) << "Method Not Found: getDataTerminalReady";
        env.checkAndClearExceptions();
        return false;
    }

    const jboolean result = env->CallStaticBooleanMethod(javaClass, methodId, static_cast<jint>(deviceId));
    if (env.checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception occurred while calling getDataTerminalReady";
        return false;
    }

    return (result == JNI_TRUE);
}

bool setDataTerminalReady(int deviceId, bool set)
{
    QJniEnvironment env;
    if (!env.isValid()) {
        qCWarning(AndroidSerialLog) << "Invalid QJniEnvironment in setDataTerminalReady";
        return false;
    }

    const jclass javaClass = getSerialManagerClass();
    if (!javaClass) {
        qCWarning(AndroidSerialLog) << "getSerialManagerClass returned null in setDataTerminalReady";
        return false;
    }

    const jmethodID methodId = env->GetStaticMethodID(javaClass, "setDataTerminalReady", "(IZ)Z");
    if (!methodId) {
        qCWarning(AndroidSerialLog) << "Method Not Found: setDataTerminalReady";
        env.checkAndClearExceptions();
        return false;
    }

    const jboolean jSet = set ? JNI_TRUE : JNI_FALSE;
    const jboolean result = env->CallStaticBooleanMethod(javaClass, methodId, static_cast<jint>(deviceId), jSet);
    if (env.checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception occurred while calling setDataTerminalReady";
        return false;
    }

    return (result == JNI_TRUE);
}

bool getRingIndicator(int deviceId)
{
    QJniEnvironment env;
    if (!env.isValid()) {
        qCWarning(AndroidSerialLog) << "Invalid QJniEnvironment in getRingIndicator";
        return false;
    }

    const jclass javaClass = getSerialManagerClass();
    if (!javaClass) {
        qCWarning(AndroidSerialLog) << "getSerialManagerClass returned null in getRingIndicator";
        return false;
    }

    const jmethodID methodId = env->GetStaticMethodID(javaClass, "getRingIndicator", "(I)Z");
    if (!methodId) {
        qCWarning(AndroidSerialLog) << "Method Not Found: getRingIndicator";
        env.checkAndClearExceptions();
        return false;
    }

    const jboolean result = env->CallStaticBooleanMethod(javaClass, methodId, static_cast<jint>(deviceId));
    if (env.checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception occurred while calling getRingIndicator";
        return false;
    }

    return (result == JNI_TRUE);
}

bool getRequestToSend(int deviceId)
{
    QJniEnvironment env;
    if (!env.isValid()) {
        qCWarning(AndroidSerialLog) << "Invalid QJniEnvironment in getRequestToSend";
        return false;
    }

    const jclass javaClass = getSerialManagerClass();
    if (!javaClass) {
        qCWarning(AndroidSerialLog) << "getSerialManagerClass returned null in getRequestToSend";
        return false;
    }

    const jmethodID methodId = env->GetStaticMethodID(javaClass, "getRequestToSend", "(I)Z");
    if (!methodId) {
        qCWarning(AndroidSerialLog) << "Method Not Found: getRequestToSend";
        env.checkAndClearExceptions();
        return false;
    }

    const jboolean result = env->CallStaticBooleanMethod(javaClass, methodId, static_cast<jint>(deviceId));
    if (env.checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception occurred while calling getRequestToSend";
        return false;
    }

    return (result == JNI_TRUE);
}

bool setRequestToSend(int deviceId, bool set)
{
    QJniEnvironment env;
    if (!env.isValid()) {
        qCWarning(AndroidSerialLog) << "Invalid QJniEnvironment in setRequestToSend";
        return false;
    }

    const jclass javaClass = getSerialManagerClass();
    if (!javaClass) {
        qCWarning(AndroidSerialLog) << "getSerialManagerClass returned null in setRequestToSend";
        return false;
    }

    const jmethodID methodId = env->GetStaticMethodID(javaClass, "setRequestToSend", "(IZ)Z");
    if (!methodId) {
        qCWarning(AndroidSerialLog) << "Method Not Found: setRequestToSend";
        env.checkAndClearExceptions();
        return false;
    }

    const jboolean jSet = set ? JNI_TRUE : JNI_FALSE;
    const jboolean result = env->CallStaticBooleanMethod(javaClass, methodId, static_cast<jint>(deviceId), jSet);
    if (env.checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception occurred while calling setRequestToSend";
        return false;
    }

    return (result == JNI_TRUE);
}

QSerialPort::PinoutSignals getControlLines(int deviceId)
{
    QJniEnvironment env;
    if (!env.isValid()) {
        qCWarning(AndroidSerialLog) << "Invalid QJniEnvironment in getControlLines";
        return QSerialPort::PinoutSignals();
    }

    const jclass javaClass = getSerialManagerClass();
    if (!javaClass) {
        qCWarning(AndroidSerialLog) << "getSerialManagerClass returned null in getControlLines";
        return QSerialPort::PinoutSignals();
    }

    const jmethodID methodId = env->GetStaticMethodID(javaClass, "getControlLines", "(I)[I");
    if (!methodId) {
        qCWarning(AndroidSerialLog) << "Method Not Found: getControlLines";
        env.checkAndClearExceptions();
        return QSerialPort::PinoutSignals();
    }

    const jintArray jarray = static_cast<jintArray>(env->CallStaticObjectMethod(javaClass, methodId, static_cast<jint>(deviceId)));
    if (!jarray) {
        qCWarning(AndroidSerialLog) << "getControlLines returned null";
        (void) env.checkAndClearExceptions();
        return QSerialPort::PinoutSignals();
    }

    if (env.checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception occurred while calling getControlLines";
        env->DeleteLocalRef(jarray);
        return QSerialPort::PinoutSignals();
    }

    jint *const ints = env->GetIntArrayElements(jarray, nullptr);
    if (!ints) {
        qCWarning(AndroidSerialLog) << "Failed to get int array elements in getControlLines";
        env->DeleteLocalRef(jarray);
        return QSerialPort::PinoutSignals();
    }

    const jsize len = env->GetArrayLength(jarray);
    QSerialPort::PinoutSignals data = QSerialPort::PinoutSignals();

    for (jsize i = 0; i < len; ++i) {
        const jint value = ints[i];
        switch (value) {
            case 0:
                data |= QSerialPort::RequestToSendSignal;
                break;
            case 1: // CTS
                data |= QSerialPort::ClearToSendSignal;
                break;
            case 2: // DTR
                data |= QSerialPort::DataTerminalReadySignal;
                break;
            case 3: // DSR
                data |= QSerialPort::DataSetReadySignal;
                break;
            case 4: // CD
                data |= QSerialPort::DataCarrierDetectSignal;
                break;
            case 5: // RI
                data |= QSerialPort::RingIndicatorSignal;
                break;
            default:
                qCWarning(AndroidSerialLog) << "Unknown ControlLine value:" << value;
                break;
        }
    }

    env->ReleaseIntArrayElements(jarray, ints, JNI_ABORT);
    env->DeleteLocalRef(jarray);
    env.checkAndClearExceptions();

    return data;
}

int getFlowControl(int deviceId)
{
    QJniEnvironment env;
    if (!env.isValid()) {
        qCWarning(AndroidSerialLog) << "Invalid QJniEnvironment in getFlowControl";
        return QSerialPort::NoFlowControl;
    }

    const jclass javaClass = getSerialManagerClass();
    if (!javaClass) {
        qCWarning(AndroidSerialLog) << "getSerialManagerClass returned null in getFlowControl";
        return QSerialPort::NoFlowControl;
    }

    const jmethodID methodId = env->GetStaticMethodID(javaClass, "getFlowControl", "(I)I");
    if (!methodId) {
        qCWarning(AndroidSerialLog) << "Method Not Found: getFlowControl";
        env.checkAndClearExceptions();
        return QSerialPort::NoFlowControl;
    }

    const jint flowControl = env->CallStaticIntMethod(javaClass, methodId, static_cast<jint>(deviceId));
    if (env.checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception occurred while calling getFlowControl";
        return QSerialPort::NoFlowControl;
    }

    return static_cast<QSerialPort::FlowControl>(flowControl);
}

bool setFlowControl(int deviceId, int flowControl)
{
    QJniEnvironment env;
    if (!env.isValid()) {
        qCWarning(AndroidSerialLog) << "Invalid QJniEnvironment in setFlowControl";
        return false;
    }

    const jclass javaClass = getSerialManagerClass();
    if (!javaClass) {
        qCWarning(AndroidSerialLog) << "getSerialManagerClass returned null in setFlowControl";
        return false;
    }

    const jmethodID methodId = env->GetStaticMethodID(javaClass, "setFlowControl", "(II)Z");
    if (!methodId) {
        qCWarning(AndroidSerialLog) << "Method Not Found: setFlowControl";
        env.checkAndClearExceptions();
        return false;
    }

    const jboolean result = env->CallStaticBooleanMethod(javaClass, methodId,
                                                         static_cast<jint>(deviceId),
                                                         static_cast<jint>(flowControl));

    if (env.checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception occurred while calling setFlowControl";
        return false;
    }

    return result == JNI_TRUE;
}

bool purgeBuffers(int deviceId, bool input, bool output)
{
    QJniEnvironment env;
    if (!env.isValid()) {
        qCWarning(AndroidSerialLog) << "Invalid QJniEnvironment in purgeBuffers";
        return false;
    }

    const jclass javaClass = getSerialManagerClass();
    if (!javaClass) {
        qCWarning(AndroidSerialLog) << "getSerialManagerClass returned null in purgeBuffers";
        return false;
    }

    const jmethodID methodId = env->GetStaticMethodID(javaClass, "purgeBuffers", "(IZZ)Z");
    if (!methodId) {
        qCWarning(AndroidSerialLog) << "Method Not Found: purgeBuffers";
        env.checkAndClearExceptions();
        return false;
    }

    const jboolean jInput = input ? JNI_TRUE : JNI_FALSE;
    const jboolean jOutput = output ? JNI_TRUE : JNI_FALSE;

    const jboolean result = env->CallStaticBooleanMethod(javaClass, methodId,
                                                         static_cast<jint>(deviceId),
                                                         jInput,
                                                         jOutput);

    if (env.checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception occurred while calling purgeBuffers";
        return false;
    }

    return (result == JNI_TRUE);
}

bool setBreak(int deviceId, bool set)
{
    QJniEnvironment env;
    if (!env.isValid()) {
        qCWarning(AndroidSerialLog) << "Invalid QJniEnvironment in setBreak";
        return false;
    }

    const jclass javaClass = getSerialManagerClass();
    if (!javaClass) {
        qCWarning(AndroidSerialLog) << "getSerialManagerClass returned null in setBreak";
        return false;
    }

    const jmethodID methodId = env->GetStaticMethodID(javaClass, "setBreak", "(IZ)Z");
    if (!methodId) {
        qCWarning(AndroidSerialLog) << "Method Not Found: setBreak";
        env.checkAndClearExceptions();
        return false;
    }

    const jboolean jSet = set ? JNI_TRUE : JNI_FALSE;
    const jboolean result = env->CallStaticBooleanMethod(javaClass, methodId,
                                                         static_cast<jint>(deviceId),
                                                         jSet);
    if (env.checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception occurred while calling setBreak";
        return false;
    }

    return (result == JNI_TRUE);
}

bool startReadThread(int deviceId)
{
    QJniEnvironment env;
    if (!env.isValid()) {
        qCWarning(AndroidSerialLog) << "Invalid QJniEnvironment in startReadThread";
        return false;
    }

    const jclass javaClass = getSerialManagerClass();
    if (!javaClass) {
        qCWarning(AndroidSerialLog) << "getSerialManagerClass returned null in startReadThread";
        return false;
    }

    const jmethodID methodId = env->GetStaticMethodID(javaClass, "startIoManager", "(I)Z");
    if (!methodId) {
        qCWarning(AndroidSerialLog) << "Method Not Found: startIoManager";
        env.checkAndClearExceptions();
        return false;
    }

    const jboolean result = env->CallStaticBooleanMethod(javaClass, methodId,
                                                         static_cast<jint>(deviceId));
    if (env.checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception occurred while calling startIoManager";
        return false;
    }

    return (result == JNI_TRUE);
}

bool stopReadThread(int deviceId)
{
    QJniEnvironment env;
    if (!env.isValid()) {
        qCWarning(AndroidSerialLog) << "Invalid QJniEnvironment in stopReadThread";
        return false;
    }

    const jclass javaClass = getSerialManagerClass();
    if (!javaClass) {
        qCWarning(AndroidSerialLog) << "getSerialManagerClass returned null in stopReadThread";
        return false;
    }

    const jmethodID methodId = env->GetStaticMethodID(javaClass, "stopIoManager", "(I)Z");
    if (!methodId) {
        qCWarning(AndroidSerialLog) << "Method Not Found: stopIoManager";
        env.checkAndClearExceptions();
        return false;
    }

    const jboolean result = env->CallStaticBooleanMethod(javaClass, methodId,
                                                         static_cast<jint>(deviceId));
    if (env.checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception occurred while calling stopIoManager";
        return false;
    }

    return (result == JNI_TRUE);
}

bool readThreadRunning(int deviceId)
{
    QJniEnvironment env;
    if (!env.isValid()) {
        qCWarning(AndroidSerialLog) << "Invalid QJniEnvironment in readThreadRunning";
        return false;
    }

    const jclass javaClass = getSerialManagerClass();
    if (!javaClass) {
        qCWarning(AndroidSerialLog) << "getSerialManagerClass returned null in readThreadRunning";
        return false;
    }

    const jmethodID methodId = env->GetStaticMethodID(javaClass, "ioManagerRunning", "(I)Z");
    if (!methodId) {
        qCWarning(AndroidSerialLog) << "Method Not Found: ioManagerRunning";
        env.checkAndClearExceptions();
        return false;
    }

    const jboolean result = env->CallStaticBooleanMethod(javaClass, methodId,
                                                         static_cast<jint>(deviceId));
    if (env.checkAndClearExceptions()) {
        qCWarning(AndroidSerialLog) << "Exception occurred while calling ioManagerRunning";
        return false;
    }

    return (result == JNI_TRUE);
}

} // namespace AndroidSerial
