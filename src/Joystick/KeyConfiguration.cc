/****************************************************************************
 *
 * Copyright (C) 2018 Pinecone Inc. All rights reserved.
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "JoystickManager.h"
#include "KeyConfiguration.h"
#include<fstream>


#define SCALE_OFFSET 874
#define SCALE_FACTOR 0.625
#define KEY_CONFIG_FILENAME "KeysConfig.ini"
#define RC_KEY_CONFIG_FILENAME "/data/rc-service/keyconfig.ini"

QString KeyConfiguration::sControlModes[] = {
    "Undefine",
    "Scroll Wheel",
    "1 Button",
    "2 Buttons",
    "3 Buttons",
    "4 Buttons",
    "5 Buttons",
    "6 buttons"
};
QString KeyConfiguration::sKeyStrings[] = {
    "Undefine",
    "1 Button",
    "2 Buttons",
    "3 Buttons",
    "4 Buttons",
    "5 Buttons",
    "6 buttons"
};
QString KeyConfiguration::sKeyNames[] = { "A", "B", "C", "D", "CAM" };

int KeyConfiguration::_deviceKeyCount = 5;;
KeyConfiguration::KeySetting_t* KeyConfiguration::_keySettingCache;
KeyConfiguration::ScrollWheelSetting_t KeyConfiguration::_scrollWheelSetting = { 0, 0 };
QStringList KeyConfiguration::_keyNameList;

KeyConfiguration::KeyConfiguration(JoystickManager* joystickManager, int channelMinNum, int channelCount, int sbus)
    : QObject()
    , _channelMinNum(channelMinNum)
    , _channelCount(channelCount)
    , _sbus(sbus)
    , _maxKeyNumPerChannel(6)
    , _channelDefaultMinValue(361)
    , _channelDefaultMaxValue(1641)
    , _sbusEnable(false)
    , _scrollWheelDefaultValue(1000)
    , _joystickManager(joystickManager)
    , _keySettingGroup("KEYSETTING")
{
    _keySettingCache = new KeySetting_t[_deviceKeyCount*2];
    memset(_keySettingCache, 0, sizeof(KeySetting_t) * _deviceKeyCount * 2);
    _configSaver = new QSettings(KEY_CONFIG_FILENAME, QSettings::IniFormat);

    for(int i = 0; i < _deviceKeyCount; i++) {
        _keyNameList << sKeyNames[i];
        _keyStringList << _keyNameList[i] + " short press";
        _keyStringList << _keyNameList[i] + " long press";
    }
    for(int i = 0; i < _maxKeyNumPerChannel + 2; i++) {
        _controlModeList << sControlModes[i];
    }
    _loadSettingToCache();
    _setChannelDefaultValues();
}

KeyConfiguration::~KeyConfiguration()
{
    delete[] _keySettingCache;
    delete _configSaver;
}

void KeyConfiguration::_loadSettingToCache()
{
    QSettings settings;
    settings.beginGroup(_keySettingGroup);
    QStringList keys = settings.childKeys();

    for (int keyIndex = 0; keyIndex < _keyStringList.size(); ++keyIndex)
    {
        settings.beginGroup(_keyStringList[keyIndex]);
        _keySettingCache[keyIndex].sbus = settings.value("sbus").toInt();
        _keySettingCache[keyIndex].channel = settings.value("channel").toInt();
        _keySettingCache[keyIndex].value = settings.value("value").toInt();
        _keySettingCache[keyIndex].switchType = settings.value("switchType").toInt();
        _keySettingCache[keyIndex].defaultValue = settings.value("defaultValue").toInt();
        settings.endGroup();

        _saveKeyConfigToFile(keyIndex);
    }
    settings.beginGroup("scrollwheel");
    _scrollWheelSetting.sbus = settings.value("sbus").toInt();
    _scrollWheelSetting.channel = settings.value("channel").toInt();
    settings.endGroup();
    _saveSWConfigToFile();
    settings.endGroup();
}

void KeyConfiguration::_setChannelDefaultValues()
{
    if (!_joystickManager->joystickMessageSender()) {
        qWarning() << "The joystickMessageSender is not ready, failed to load set default values!";
        return;
    }
    QMap<int, int> map;
    int channel;
    for (int i = 0; i < _deviceKeyCount * 2; ++i)
    {
        channel = _keySettingCache[i].channel;
        if( channel!= 0) {
            if(_keySettingCache[i].sbus == _sbus) {
                if (_keySettingCache[i].switchType > 0) {
                    _joystickManager->joystickMessageSender()->setChannelValue(_sbus, channel, _keySettingCache[i].defaultValue);
                } else {
                    if((map.contains(channel) && map[channel] > _keySettingCache[i].value) ||
                       !map.contains(channel)) {
                        map[channel] = _keySettingCache[i].value;
                    }
                }
            }
        }
    }
    if (map.size() > 0) {
        QMap<int, int>::iterator it;
        for (it = map.begin(); it != map.end(); ++it) {
            _joystickManager->joystickMessageSender()->setChannelValue(_sbus, it.key(), it.value());
        }
    }
    if(_scrollWheelSetting.sbus > 0 && _scrollWheelSetting.channel > 0) {
        _joystickManager->joystickMessageSender()->setChannelValue(_scrollWheelSetting.sbus, _scrollWheelSetting.channel, _scrollWheelDefaultValue);
    }
}

void KeyConfiguration::setChannelDefaultValue(int sbus, int channel)
{
    int v = 0;
    for (int i = 0; i < _deviceKeyCount * 2; ++i)
    {
        if (channel == _keySettingCache[i].channel) {
            if(_keySettingCache[i].sbus == sbus) {
                if (_keySettingCache[i].switchType > 0) {
                    _joystickManager->joystickMessageSender()->setChannelValue(sbus, channel, _keySettingCache[i].defaultValue);
                    return;
                } else {
                    if(v > _keySettingCache[i].value || v == 0) {
                        v = _keySettingCache[i].value;
                    }
                }
            }
        }
    }
    _joystickManager->joystickMessageSender()->setChannelValue(sbus, channel, v);
    emit channelValueCountsChanged();
}

int KeyConfiguration::getSeqInChannel(int channel, int value)
{
    QMap<int, int> map;
    for (int i = 0; i < _deviceKeyCount * 2; ++i)
    {
        if(channel == _keySettingCache[i].channel && _sbus == _keySettingCache[i].sbus) {
            if (_keySettingCache[i].switchType > 0) {
                if (value == _keySettingCache[i].defaultValue) {
                    return value >  _keySettingCache[i].value ? 2 : 1;
                } else if (value == _keySettingCache[i].value) {
                    return value >  _keySettingCache[i].defaultValue ? 2 : 1;
                }
            } else {
                map[_keySettingCache[i].value] = i;
            }
        }
    }
    if (map.size() > 0) {
        QMap<int, int>::iterator it;
        int seq = 1;
        for (it = map.begin(); it != map.end(); ++it) {
            if (it.key() == value) {
                return seq;
            }
            seq++;
        }
    }
    return 0;
}

int KeyConfiguration::getChannelValueCount(int channel)
{
    QMap<int, int> map;
    for (int i = 0; i < _deviceKeyCount * 2; ++i)
    {
        if(_sbus == _keySettingCache[i].sbus && channel == _keySettingCache[i].channel) {
            if (_keySettingCache[i].switchType > 0) {
                return 2;
            } else {
                map[_keySettingCache[i].value] = i;
            }
        }
    }
    return map.size();
}

QVariantList KeyConfiguration::channelValueCounts()
{
    QVariantList list;

    for (int i = _channelMinNum; i < _channelCount + _channelMinNum; i++) {
        list += QVariant::fromValue(getChannelValueCount(i));
    }
    return list;
}

bool KeyConfiguration::getChannelValue(int keyCode, KeyAction_t action, int* sbus, int* channel, int* value)
{
    int index = getKeyIndexFromKeyCode(keyCode, action);
    if (index < 0 || index >= 2 * _deviceKeyCount) {
        return false;
    }
    if (_keySettingCache[index].switchType != 2 &&
        (action == keyAction_down || action == keyAction_up)) {
        return false;
    }
    *channel = _keySettingCache[index].channel;
    *sbus = _keySettingCache[index].sbus;
    if (_keySettingCache[index].switchType == 2) {
        if (action == keyAction_down) {
            *value = _keySettingCache[index].value;
        } else if (action == keyAction_up) {
            *value = _keySettingCache[index].defaultValue;
        } else {
            return false;
        }
    } else if (_keySettingCache[index].switchType == 1) {
        if (currentChannelValue(_keySettingCache[index].sbus, _keySettingCache[index].channel) == _keySettingCache[index].value) {
            *value = _keySettingCache[index].defaultValue;
        } else {
            *value = _keySettingCache[index].value;
        }
    } else {
        *value = _keySettingCache[index].value;
    }
    return (*channel != 0);
}

bool KeyConfiguration::getScrollWheelSetting(int *sbus, int *channel)
{
    *sbus = _scrollWheelSetting.sbus;
    *channel = _scrollWheelSetting.channel;

    return (*sbus != 0);
}

int KeyConfiguration::currentChannelValue(int sbus, int channel)
{
    return JoystickMessageSender::getChannelValue(sbus, channel);
}

int KeyConfiguration::getKeyIndexFromKeyCode(int keyCode, int action)
{
    int index = -1;
    // to check hold mode setting, just use the setting for short
    if (action == KeyConfiguration::keyAction_down || action == KeyConfiguration::keyAction_up) {
        action = 0;
    }
    if(keyCode == 121) // KEYCODE_BREAK
    {
        index = _keyNameList.indexOf("A") * 2 + action;
    }
    else if(keyCode == 4) // KEYCODE_BACK
    {
        index = _keyNameList.indexOf("B") * 2 + action;
    }
    else if(keyCode == 24) // KEYCODE_VOLUME_UP
    {
        index = _keyNameList.indexOf("C") * 2 + action;
    }
    else if(keyCode == 25) // KEYCODE_VOLUME_DOWN
    {
        index = _keyNameList.indexOf("D") * 2 + action;
    }
    else if(keyCode == 27) // KEYCODE_CAMERA
    {
        index = _keyNameList.indexOf("CAM") * 2 + action;
    }
    return index;
}

void KeyConfiguration::saveKeySetting(int keyIndex, int channel, int value)
{
    _keySettingCache[keyIndex].sbus = _sbus;
    _keySettingCache[keyIndex].channel = channel;
    _keySettingCache[keyIndex].value = value;
    _keySettingCache[keyIndex].switchType = 0;
    _keySettingCache[keyIndex].defaultValue = 0;

    _saveKeyConfiguration(keyIndex);

    emit channelKeyCountsChanged();
    emit keySettingStringsChanged();
}

void KeyConfiguration::saveSingleKeySetting(int keyIndex,
                                            int switchType,
                                            int channel,
                                            int value,
                                            int defaultValue)
{
    if ((switchType > 15) || (channel > _channelMinNum + _channelCount) || (value > 4095) || (defaultValue > 4095)) {
        qWarning("input contains invalid value");
        return;
    }
    _keySettingCache[keyIndex].sbus = _sbus;
    _keySettingCache[keyIndex].channel = channel;
    _keySettingCache[keyIndex].value = value;
    _keySettingCache[keyIndex].switchType = switchType;
    _keySettingCache[keyIndex].defaultValue = defaultValue;

    _saveKeyConfiguration(keyIndex);
    if(switchType == 2) { //hold mode will use both short and long press of the key
        int otherKeyIndex = (keyIndex % 2 == 0) ? (keyIndex + 1) : (keyIndex - 1);
        memcpy(&_keySettingCache[otherKeyIndex], &_keySettingCache[keyIndex], sizeof(KeySetting_t));

        _saveKeyConfiguration(otherKeyIndex);
    }
    qDebug() << "single key setting is stored";
    emit channelKeyCountsChanged();
    emit keySettingStringsChanged();
}

void KeyConfiguration::saveScollWheelSetting(int channel)
{
    _scrollWheelSetting.sbus = _sbus;
    _scrollWheelSetting.channel = channel;

    _saveScrollWheelConfiguration();
    _joystickManager->joystickMessageSender()->setChannelValue(_scrollWheelSetting.sbus, _scrollWheelSetting.channel, _scrollWheelDefaultValue);

    emit channelKeyCountsChanged();
    emit keySettingStringsChanged();
}

void KeyConfiguration::_saveKeyConfiguration(int keyIndex)
{
    QSettings settings;

    settings.beginGroup(_keySettingGroup);
    settings.beginGroup(_keyStringList[keyIndex]);
    settings.setValue("sbus", _keySettingCache[keyIndex].sbus);
    settings.setValue("channel", _keySettingCache[keyIndex].channel);
    settings.setValue("value", _keySettingCache[keyIndex].value);
    settings.setValue("switchType", _keySettingCache[keyIndex].switchType);
    settings.setValue("defaultValue", _keySettingCache[keyIndex].defaultValue);
    settings.endGroup();
    settings.endGroup();

    _saveKeyConfigToFile(keyIndex);
}

void KeyConfiguration::_saveKeyConfigToFile(int keyIndex)
{
    _configSaver->beginGroup(QString(_keyStringList[keyIndex]).replace(" ", "_"));
    _configSaver->setValue("sbus", _keySettingCache[keyIndex].sbus);
    _configSaver->setValue("channel", _keySettingCache[keyIndex].channel);
    _configSaver->setValue("value", _keySettingCache[keyIndex].value);
    _configSaver->setValue("switchType", _keySettingCache[keyIndex].switchType);
    _configSaver->setValue("defaultValue", _keySettingCache[keyIndex].defaultValue);
    _configSaver->endGroup();
    _configSaver->sync();

    std::ifstream infile(KEY_CONFIG_FILENAME);
    std::ofstream outfile(RC_KEY_CONFIG_FILENAME);
    char buf[2048];
    while(infile) {
        infile.read(buf, 2048);
        outfile.write(buf, infile.gcount());
    }

    infile.close();
    outfile.close();
}

void KeyConfiguration::_saveScrollWheelConfiguration()
{
    QSettings settings;

    settings.beginGroup(_keySettingGroup);
    settings.beginGroup("scrollwheel");
    settings.setValue("sbus", _scrollWheelSetting.sbus);
    settings.setValue("channel", _scrollWheelSetting.channel);
    settings.endGroup();
    settings.endGroup();

    _saveSWConfigToFile();
}

void KeyConfiguration::_saveSWConfigToFile()
{
    _configSaver->beginGroup("scrollwheel");
    _configSaver->setValue("sbus", _scrollWheelSetting.sbus);
    _configSaver->setValue("channel", _scrollWheelSetting.channel);
    _configSaver->endGroup();
    _configSaver->sync();

    std::ifstream infile(KEY_CONFIG_FILENAME);
    std::ofstream outfile(RC_KEY_CONFIG_FILENAME);
    char buf[2048];
    while(infile) {
        infile.read(buf, 2048);
        outfile.write(buf, infile.gcount());
    }

    infile.close();
    outfile.close();
}

int KeyConfiguration::ppmToSbus(int ppm) {
    int sbus;

    if(ppm < 800 || ppm > 2200) {
        qWarning() << "ppm value out of range";
        return -1;
    }
    if(ppm >= 800 && ppm <= 874) {
        sbus = 0;
    } else if(ppm >= 875 && ppm <= 2152) {
        sbus = ceil((ppm - SCALE_OFFSET - 0.5f) / SCALE_FACTOR);
    } else if(ppm >= 2153 && ppm <= 2200) {
        sbus = 2047;
    }

    return sbus;
}

int KeyConfiguration::sbusToPPM(int sbus) {
    int ppm;

    if(sbus < 0 || sbus > 2047) {
        qWarning() << "sbus value out of range";
        return -1;
    }
    if(sbus == 0) {
        ppm = 800;
    } else if(sbus == 2047) {
        ppm = 2200;
    } else {
        ppm = sbus * SCALE_FACTOR + SCALE_OFFSET + 0.5f;
    }

    return ppm;
}

int KeyConfiguration::channelOnKey(int keyIndex, int switchType)
{
    int ch = _keySettingCache[keyIndex].channel;
    if (switchType == 2 && ch == 0) {
        int otherKeyIndex = (keyIndex % 2 == 0) ? (keyIndex + 1) : (keyIndex - 1);
        ch = _keySettingCache[otherKeyIndex].channel;
    }
    return ch;
}

int KeyConfiguration::sbusOnKey(int keyIndex, int switchType)
{
    int sbus = _keySettingCache[keyIndex].sbus;
    if (switchType == 2 && sbus == 0) {
        int otherKeyIndex = (keyIndex % 2 == 0) ? (keyIndex + 1) : (keyIndex - 1);
        sbus = _keySettingCache[otherKeyIndex].sbus;
    }
    return sbus;
}

int KeyConfiguration::channelCount()
{
    return _channelCount;
}

int KeyConfiguration::getControlMode(int channel)
{
    int count = 0;
    QString conMode;
    for (int i = 0; i < _deviceKeyCount * 2; ++i)
    {
        if(_keySettingCache[i].channel == channel && _keySettingCache[i].sbus == _sbus) {
            if (_keySettingCache[i].switchType != 0) {
                count = 1;
                break;
            }
            count++;
        }
    }
    conMode = sKeyStrings[count];
    if(_scrollWheelSetting.sbus == _sbus && _scrollWheelSetting.channel == channel) {
        conMode = "Scroll Wheel";
    }
    return _controlModeList.indexOf(conMode);
}

int KeyConfiguration::getControlModeByKeyCount(int keyCount)
{
    return _controlModeList.indexOf(sKeyStrings[keyCount]);
}

QVariantList KeyConfiguration::channelKeyCounts()
{
    QVariantList list;

    for(int i = _channelMinNum; i < _channelCount + _channelMinNum; i++) {
        list += QVariant::fromValue(getControlMode(i));
    }
    return list;
}

QVariantList KeyConfiguration::keySettingStrings()
{
    QVariantList list;

    for(int i = _channelMinNum; i < _channelCount + _channelMinNum; i++) {
        list += QVariant::fromValue(getKeySettingString(i));
    }
    return list;
}

QString KeyConfiguration::getKeySettingString(int channel)
{
    QString keyString;
    QMap<int, int> map;
    for (int i = 0; i < _deviceKeyCount * 2; ++i)
    {
        if(_keySettingCache[i].sbus == _sbus) {
            if(_keySettingCache[i].channel == channel) {
                if (_keySettingCache[i].switchType == 2) {
                    keyString = getKeyNameFromIndex(i) + ", " + "Momentary switch";
                    return keyString;
                } else if (_keySettingCache[i].switchType == 1) {
                    keyString = getKeyStringFromIndex(i) + ", " + "Toggle switch";
                    return keyString;
                } else {
                    map[_keySettingCache[i].value] = i;
                }
            }
        }
    }
    if (map.size() > 0) {
        QMap<int, int>::iterator it;
        for (it = map.begin(); it != map.end(); ++it) {
            if(!keyString.isEmpty()) {
                keyString += ", ";
            }
            keyString += getKeyStringFromIndex(it.value());
        }
    }
    if(_scrollWheelSetting.sbus == _sbus && _scrollWheelSetting.channel == channel) {
        keyString = "Scroll Wheel";
    }
    if(keyString.isEmpty()) {
        keyString = "Undefined";
    }
    return keyString;
}

bool KeyConfiguration::sbusEnable()
{
    return _sbusEnable;
}

void KeyConfiguration::setSbusEnable(bool sbusEnable)
{
    _sbusEnable = sbusEnable;

    emit sbusEnableChanged();
}

int KeyConfiguration::getKeyIndex(int channel, int seq, int count)
{
    QMap<int, int> map;
    for (int i = 0; i < _deviceKeyCount * 2; ++i)
    {
        if(_keySettingCache[i].channel == channel) {
            if(_keySettingCache[i].sbus == _sbus) {
                count--;
                map[_keySettingCache[i].value] = i;
                if (count <= 0) {
                    break;
                }
            }
        }
    }
    if (map.size() > 0) {
        QMap<int, int>::iterator it;
        for (it = map.begin(); it != map.end(); ++it) {
            seq--;
            if (seq == 0) {
                return it.value();
            }
        }
    }
    return 0;
}

int KeyConfiguration::getValue(int channel, int seq, int count)
{
    QMap<int, int> map;
    for (int i = 0; i < _deviceKeyCount * 2; ++i)
    {
        if(_keySettingCache[i].channel == channel) {
            if(_keySettingCache[i].sbus == _sbus) {
                count--;
                map[_keySettingCache[i].value] = i;
                if (count <= 0) {
                    break;
                }
            }
        }
    }
    if (map.size() > 0) {
        QMap<int, int>::iterator it;
        for (it = map.begin(); it != map.end(); ++it) {
            seq--;
            if (seq == 0) {
                return it.key();
            }
        }
    }
    if (count < 1) {
        return 0;
    }
    if (count == 1) {
        return _channelDefaultMaxValue;
    }
    return _channelDefaultMinValue + (seq-1) * (_channelDefaultMaxValue-_channelDefaultMinValue) / (count-1);
}

int KeyConfiguration::getDefaultValue(int channel)
{
    for (int i = 0; i < _deviceKeyCount * 2; ++i)
    {
        if(_keySettingCache[i].channel == channel) {
            if(_keySettingCache[i].sbus == _sbus) {
                return _keySettingCache[i].defaultValue;
            }
        }
    }
    return _channelDefaultMinValue;
}

int KeyConfiguration::getSwitchType(int channel)
{
    for (int i = 0; i < _deviceKeyCount * 2; ++i)
    {
        if(_keySettingCache[i].channel == channel) {
            if(_keySettingCache[i].sbus == _sbus) {
                return _keySettingCache[i].switchType;
            }
        }
    }
    return 0;
}

int KeyConfiguration::getChannelMinNum()
{
    return _channelMinNum;
}

void KeyConfiguration::resetKeySetting(int sbus, int channel)
{
    for (int i = 0; i < _deviceKeyCount * 2; ++i)
    {
        if(_keySettingCache[i].channel == channel) {
            if(_keySettingCache[i].sbus == sbus) {
                memset(&_keySettingCache[i], 0 ,sizeof(KeySetting_t));
                _saveKeyConfiguration(i);
            }
        }
    }
    if(_scrollWheelSetting.sbus == sbus && _scrollWheelSetting.channel == channel) {
        resetScrollWheelSetting();
    }
    setChannelDefaultValue(sbus, channel);
    emit channelKeyCountsChanged();
    emit keySettingStringsChanged();
}

void KeyConfiguration::resetScrollWheelSetting()
{
    if(_scrollWheelSetting.sbus > 0 && _scrollWheelSetting.channel > 0) {
        _joystickManager->joystickMessageSender()->setChannelValue(_scrollWheelSetting.sbus, _scrollWheelSetting.channel, 0);
    }
    _scrollWheelSetting.sbus = 0;
    _scrollWheelSetting.channel = 0;

    _saveScrollWheelConfiguration();
}

QStringList KeyConfiguration::availableKeys()
{
    return _keyStringList;
}

QString KeyConfiguration::getKeyStringFromIndex(int index)
{
    if (index < _keyStringList.count()) {
        return _keyStringList[index];
    }
    return "";
}

QString KeyConfiguration::getKeyNameFromIndex(int index)
{
    if (index / 2  < _keyNameList.count()) {
        return _keyNameList[index/2];
    }
    return "";
}

QStringList KeyConfiguration::availableControlModes()
{
    return _controlModeList;
}
