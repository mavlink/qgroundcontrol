/****************************************************************************
 *
 * Copyright (C) 2018 Pinecone Inc. All rights reserved.
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "WifiSettings.h"
#include "QGCApplication.h"
#include "QGCToolbox.h"
#include "SettingsManager.h"
#include "VideoSettings.h"

#include <QDebug>
#include <QQmlEngine>
#include <QQmlApplicationEngine>

#include <QtAndroidExtras/QAndroidJniObject>
#include <QtAndroidExtras/QAndroidJniEnvironment>
#include <jni.h>

enum WifiApState {
    WIFI_AP_STATE_DISABLED = 0,
    WIFI_AP_STATE_ENABLED
};

const char* WifiSettings::_rtspURL = "rtsp://192.168.43.1:8554/fpv_stream";

WifiSettings::WifiSettings()
    : QObject()
{
}

QString WifiSettings::videoShareSSID(void)
{
    QAndroidJniObject wifiConfig = QAndroidJniObject::callStaticObjectMethod("org.mavlink.qgroundcontrol.QGCActivity", "getWifiApConfiguration", "()Landroid/net/wifi/WifiConfiguration;");
    QAndroidJniObject SSID = wifiConfig.getObjectField<jstring>("SSID");
    return SSID.toString();
}

int WifiSettings::videoShareAuthType(void)
{
    QAndroidJniObject wifiConfig = QAndroidJniObject::callStaticObjectMethod("org.mavlink.qgroundcontrol.QGCActivity", "getWifiApConfiguration", "()Landroid/net/wifi/WifiConfiguration;");
    int authType = wifiConfig.callMethod<jint>("getAuthType");
    if(authType > 0)
        return 1;
    return authType;
}

QString WifiSettings::videoSharePasswd(void)
{
    QAndroidJniObject wifiConfig = QAndroidJniObject::callStaticObjectMethod("org.mavlink.qgroundcontrol.QGCActivity", "getWifiApConfiguration", "()Landroid/net/wifi/WifiConfiguration;");
    QAndroidJniObject passwd = wifiConfig.getObjectField<jstring>("preSharedKey");
    return passwd.toString();
}

QString WifiSettings::rtspURL(void)
{
    return QString(_rtspURL);
}

bool WifiSettings::setVideoShareApConfig(QString name, QString passwd, int authType, bool restart)
{
    bool ret;
    QAndroidJniObject apName = QAndroidJniObject::fromString(name);
    QAndroidJniObject apPasswd = QAndroidJniObject::fromString(passwd);
    ret = QAndroidJniObject::callStaticMethod<jboolean>("org.mavlink.qgroundcontrol.QGCActivity", "setWifiApConfiguration",
                                "(Ljava/lang/String;Ljava/lang/String;I)Z", apName.object<jstring>(), apPasswd.object<jstring>(), authType);

    if(restart) {
        QAndroidJniObject::callStaticMethod<void>("org.mavlink.qgroundcontrol.QGCActivity", "setNeedRestartWifiAp", "(Z)V", restart);
        ret = QAndroidJniObject::callStaticMethod<jboolean>("org.mavlink.qgroundcontrol.QGCActivity", "setWifiApEnabled", "(Z)Z", false);
    }

    if(!ret)
        qWarning() << "Set wifi AP configuration failed.";
    return ret;
}

bool WifiSettings::setVideoShareApEnabled(bool enabled)
{
    bool ret = QAndroidJniObject::callStaticMethod<jboolean>("org.mavlink.qgroundcontrol.QGCActivity", "setWifiApEnabled", "(Z)Z", enabled);
    return ret;
}

void WifiSettings::setCountryCode(QString country, bool persist)
{
    QAndroidJniObject countryObj = QAndroidJniObject::fromString(country);
    QAndroidJniObject::callStaticMethod<void>("org.mavlink.qgroundcontrol.QGCActivity", "setCountryCode", "(Ljava/lang/String;Z)V", countryObj.object<jstring>(), persist);
}

QString WifiSettings::getCountryCode()
{
    QAndroidJniObject countryCode = QAndroidJniObject::callStaticObjectMethod("org.mavlink.qgroundcontrol.QGCActivity", "getCountryCode", "()Ljava/lang/String;");
    return countryCode.toString();
}

//native method called by java, receive wifi ap state from android broadcast
static void sendWifiApState(JNIEnv* /*env*/, jobject /*thiz*/, jint state)
{
    if(state == 11) {//disabled
        if(qgcApp())
            emit qgcApp()->toolbox()->settingsManager()->videoSettings()->videoShareSettings()->wifiAPStateChanged(WIFI_AP_STATE_DISABLED);
    } else if(state == 13) {//enabled
        if(qgcApp())
            emit qgcApp()->toolbox()->settingsManager()->videoSettings()->videoShareSettings()->wifiAPStateChanged(WIFI_AP_STATE_ENABLED);
    }
}

void WifiSettings::setNativeMethods(void)
{
    JNINativeMethod methods[] = {
        {"nativeSendWifiApState", "(I)V", (void *)sendWifiApState}
    };

    const char * classname = "org/mavlink/qgroundcontrol/QGCActivity";
    QAndroidJniEnvironment jniEnv;
    jclass clazz = jniEnv->FindClass(classname);
    if(!clazz) {
        qWarning() << "Couldn't find class:" << classname;
        return;
    }

    jint ret = jniEnv->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0]));
    if (ret < 0) {
        qWarning() << "Error registering methods: " << ret;
    }
    QAndroidJniObject::callStaticMethod<void>("org.mavlink.qgroundcontrol.QGCActivity", "registerBroadcast", "()V");
}

