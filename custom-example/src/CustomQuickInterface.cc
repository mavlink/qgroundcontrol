/****************************************************************************
 *
 * (c) 2009-2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 * @file
 *   @brief Custom QtQuick Interface
 *   @author Gus Grubba <gus@auterion.com>
 */

#include "QGCApplication.h"
#include "AppSettings.h"
#include "SettingsManager.h"
#include "MAVLinkLogManager.h"
#include "QGCMapEngine.h"
#include "QGCApplication.h"
#include "PositionManager.h"

#include "CustomPlugin.h"
#include "CustomQuickInterface.h"

#include <QDirIterator>
#include <QtAlgorithms>

//-----------------------------------------------------------------------------
CustomQuickInterface::CustomQuickInterface(QObject* parent)
    : QObject(parent)
{
    qCDebug(CustomLog) << "CustomQuickInterface Created";
}

//-----------------------------------------------------------------------------
CustomQuickInterface::~CustomQuickInterface()
{
    qCDebug(CustomLog) << "CustomQuickInterface Destroyed";
}

//-----------------------------------------------------------------------------
void
CustomQuickInterface::init()
{
}
