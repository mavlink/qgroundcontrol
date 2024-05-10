/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "EsriMapProvider.h"
#include "QGCApplication.h"
#include "SettingsManager.h"

QByteArray EsriMapProvider::getToken() const
{
    // TODO: Make this a QObject and connect to appsettings changed?
    const QByteArray result(qgcApp()->toolbox()->settingsManager()->appSettings()->esriToken()->rawValue().toString().toLatin1());
    return result;
}
