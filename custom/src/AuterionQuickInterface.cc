/*!
 * @file
 *   @brief Auterion QtQuick Interface
 *   @author Gus Grubba <gus@grubba.com>
 */

#include "QGCApplication.h"
#include "AppSettings.h"
#include "SettingsManager.h"
#include "MAVLinkLogManager.h"
#include "QGCMapEngine.h"

#include "AuterionPlugin.h"
#include "AuterionQuickInterface.h"

#include <QDirIterator>
#include <QtAlgorithms>

//-----------------------------------------------------------------------------
AuterionQuickInterface::AuterionQuickInterface(QObject* parent)
    : QObject(parent)
{
    qCDebug(AuterionLog) << "AuterionQuickInterface Created";
}

//-----------------------------------------------------------------------------
AuterionQuickInterface::~AuterionQuickInterface()
{
    qCDebug(AuterionLog) << "AuterionQuickInterface Destroyed";
}

//-----------------------------------------------------------------------------
void
AuterionQuickInterface::init()
{
}
