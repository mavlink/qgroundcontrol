/****************************************************************************
 *
 * Copyright (C) 2018 Pinecone Inc. All rights reserved.
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "Joystick.h"

class JoystickManager;

class KeyConfiguration : public QObject
{
    Q_OBJECT

public:
    typedef enum {
        keyAction_shortPress,
        keyAction_longPress,
        keyAction_down,
        keyAction_up,
    } KeyAction_t;

    typedef struct
    {
        int sbus;
        int channel;
        int value;
        int switchType;
        int defaultValue;
    } KeySetting_t;

    typedef struct
    {
        int sbus;
        int channel;
    } ScrollWheelSetting_t;

    KeyConfiguration(JoystickManager* joystickManager, int channelMinNum, int channelCount, int sbus);
    ~KeyConfiguration();

    Q_PROPERTY(int channelCount READ channelCount CONSTANT)
    Q_PROPERTY(bool sbusEnable READ sbusEnable WRITE setSbusEnable NOTIFY sbusEnableChanged)
    Q_PROPERTY(QVariantList channelValueCounts READ channelValueCounts NOTIFY channelValueCountsChanged)
    Q_PROPERTY(QVariantList channelKeyCounts READ channelKeyCounts NOTIFY channelKeyCountsChanged)
    Q_PROPERTY(QVariantList keySettingStrings READ keySettingStrings NOTIFY keySettingStringsChanged)
    Q_PROPERTY(QStringList availableKeys READ availableKeys CONSTANT)
    Q_PROPERTY(QStringList availableControlModes READ availableControlModes CONSTANT)

    Q_INVOKABLE QString  getKeyStringFromIndex(int index);
    Q_INVOKABLE QString  getKeyNameFromIndex(int index);
    Q_INVOKABLE void saveKeySetting(int keyIndex, int channel, int value);
    Q_INVOKABLE void saveSingleKeySetting(int keyIndex,
                                          int switchType,
                                          int channel,
                                          int value,
                                          int defaultValue);
    Q_INVOKABLE void saveScollWheelSetting(int channel);
    Q_INVOKABLE int sbusOnKey(int keyIndex, int switchType);
    Q_INVOKABLE int channelOnKey(int keyIndex, int switchType);
    Q_INVOKABLE int getControlMode(int channel);
    Q_INVOKABLE int getControlModeByKeyCount(int keyCount);
    Q_INVOKABLE QString getKeySettingString(int channel);
    Q_INVOKABLE void resetKeySetting(int sbus, int channel);
    Q_INVOKABLE void resetScrollWheelSetting();
    Q_INVOKABLE int getKeyIndex(int channel, int seq, int count);
    Q_INVOKABLE int getValue(int channel, int seq, int count);
    Q_INVOKABLE int getDefaultValue(int channel);
    Q_INVOKABLE int getSwitchType(int channel);
    Q_INVOKABLE int getChannelMinNum();
    Q_INVOKABLE void setChannelDefaultValue(int sbus, int channel);
    Q_INVOKABLE int ppmToSbus(int ppm);
    Q_INVOKABLE int sbusToPPM(int sbus);

    int getSeqInChannel(int channel, int value);
    int getChannelValueCount(int channel);

    static bool getChannelValue(int keyCode, KeyAction_t action, int* sbus, int* channel, int* value);
    static bool getScrollWheelSetting(int *sbus, int *channel);
    int channelCount();
    bool sbusEnable();
    void setSbusEnable(bool sbusEnable);
    QVariantList channelValueCounts();
    QVariantList channelKeyCounts();
    QVariantList keySettingStrings();
    QStringList availableKeys();
    QStringList availableControlModes();

    static QString sControlModes[];
    static QString sKeyStrings[];
    static QString sKeyNames[];

signals:
    void channelValueCountsChanged();
    void channelKeyCountsChanged();
    void keySettingStringsChanged();
    void sbusEnableChanged();

private:
    static int currentChannelValue(int sbus, int channel);
    static int getKeyIndexFromKeyCode(int keyCode, int action);
    void _loadSettingToCache();
    void _setChannelDefaultValues();
    void _saveKeyConfiguration(int keyIndex);
    void _saveKeyConfigToFile(int keyIndex);
    void _saveScrollWheelConfiguration();
    void _saveSWConfigToFile();

    static int _deviceKeyCount;
    static KeySetting_t* _keySettingCache;
    static ScrollWheelSetting_t _scrollWheelSetting;
    static QStringList _keyNameList;
    int _channelMinNum;
    int _channelCount;
    int _sbus;
    int _maxKeyNumPerChannel;
    int _channelDefaultMinValue;
    int _channelDefaultMaxValue;
    bool _sbusEnable;
    int _scrollWheelDefaultValue;
    QStringList chBtnListQStr;
    JoystickManager* _joystickManager;
    QString _keySettingGroup;
    QStringList _keyStringList;
    QStringList _controlModeList;
    QSettings *_configSaver;
};
