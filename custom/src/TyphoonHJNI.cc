/*!
 * @file
 *   @brief ST16 Android JNI Interface
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

/*-----------------------------------------------------------------------------
 *   Original source:
 *
 *   DroneFly/droneservice/src/main/java/com/yuneec/droneservice/parse/St16Controller.java
 *
 *   All comments within the command send functions came from the original file above.
 *   The functions themselves have been completely rewriten from scratch.
 */

#include "TyphoonHCommon.h"
#include "TyphoonHM4Interface.h"

#include <jni.h>
#include <QtAndroidExtras/QtAndroidExtras>
#include <QtAndroidExtras/QAndroidJniObject>

const char* jniClassName = "org/qgroundcontrol/qgchelper/UsbDeviceJNI";

//-----------------------------------------------------------------------------
void
jniSetup(JNIEnv *envA, jobject thizA)
{
    Q_UNUSED(thizA);
    if(envA->ExceptionCheck())
        envA->ExceptionClear();
}

//-----------------------------------------------------------------------------
static void
jniNewWifiItem(JNIEnv *envA, jobject thizA, jstring jSsid)
{
    jniSetup(envA, thizA);
    if(TyphoonHM4Interface::pTyphoonHandler) {
        const char *stringL = envA->GetStringUTFChars(jSsid, NULL);
        QString ssid = QString::fromUtf8(stringL);
        envA->ReleaseStringUTFChars(jSsid, stringL);
        emit TyphoonHM4Interface::pTyphoonHandler->newWifiSSID(ssid);
    }
}

//-----------------------------------------------------------------------------
static void
jniScanComplete(JNIEnv *envA, jobject thizA)
{
    jniSetup(envA, thizA);
    if(TyphoonHM4Interface::pTyphoonHandler) {
        emit TyphoonHM4Interface::pTyphoonHandler->scanComplete();
    }
}

//-----------------------------------------------------------------------------
static void
jniAuthError(JNIEnv *envA, jobject thizA)
{
    jniSetup(envA, thizA);
    if(TyphoonHM4Interface::pTyphoonHandler) {
        emit TyphoonHM4Interface::pTyphoonHandler->authenticationError();
    }
}

//-----------------------------------------------------------------------------
static void
jniWifiConnected(JNIEnv *envA, jobject thizA)
{
    jniSetup(envA, thizA);
    if(TyphoonHM4Interface::pTyphoonHandler) {
        emit TyphoonHM4Interface::pTyphoonHandler->wifiConnected();
    }
}

//-----------------------------------------------------------------------------
JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* /*reserved*/)
{
    //-- Register C++ functions exposed to Android
    static JNINativeMethod javaMethods[] {
        {"nativeNewWifiItem",  "(Ljava/lang/String;)V", reinterpret_cast<void *>(jniNewWifiItem)},
        {"nativeScanComplete", "()V", reinterpret_cast<void *>(jniScanComplete)},
        {"nativeAuthError",    "()V", reinterpret_cast<void *>(jniAuthError)},
        {"nativeWifiConnected","()V", reinterpret_cast<void *>(jniWifiConnected)}
    };
    JNIEnv* env;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    jclass javaClass = env->FindClass(jniClassName);
    if(!javaClass) {
        qCWarning(YuneecLog) << "Couldn't find class:" << jniClassName;
        return JNI_ERR;
    }
    if (env->RegisterNatives(javaClass, javaMethods,  sizeof(javaMethods) / sizeof(javaMethods[0])) < 0) {
        qCWarning(YuneecLog) << "Couldn't register native methods";
        return JNI_ERR;
    }
    return JNI_VERSION_1_6;
}
