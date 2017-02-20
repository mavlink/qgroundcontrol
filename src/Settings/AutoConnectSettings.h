/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifndef AutoConnectSettings_H
#define AutoConnectSettings_H

#include "SettingsGroup.h"

class AutoConnectSettings : public SettingsGroup
{
    Q_OBJECT
    
public:
    AutoConnectSettings(QObject* parent = NULL);

    static const char* autoConnectSettingsGroupName;

private:
};

#endif
