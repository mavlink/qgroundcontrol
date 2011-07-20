/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2010 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief Implementation of class UASManager
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <QList>
#include <QApplication>
#include <QMessageBox>
#include <QTimer>
#include <QSettings>
#include "UAS.h"
#include "UASInterface.h"
#include "UASManager.h"
#include "QGC.h"

UASManager* UASManager::instance()
{
    static UASManager* _instance = 0;
    if(_instance == 0) {
        _instance = new UASManager();

        // Set the application as parent to ensure that this object
        // will be destroyed when the main application exits
        _instance->setParent(qApp);
    }
    return _instance;
}

void UASManager::storeSettings()
{
    QSettings settings;
    settings.beginGroup("QGC_UASMANAGER");
    settings.setValue("HOMELAT", homeLat);
    settings.setValue("HOMELON", homeLon);
    settings.setValue("HOMEALT", homeAlt);
    settings.endGroup();
    settings.sync();
}

void UASManager::loadSettings()
{
    QSettings settings;
    settings.sync();
    settings.beginGroup("QGC_UASMANAGER");
    bool changed =  setHomePosition(settings.value("HOMELAT", homeLat).toDouble(),
                    settings.value("HOMELON", homeLon).toDouble(),
                    settings.value("HOMEALT", homeAlt).toDouble());

    // Make sure to fire the change - this will
    // make sure widgets get the signal once
    if (!changed)
    {
        emit homePositionChanged(homeLat, homeLon, homeAlt);
    }

    settings.endGroup();
}

bool UASManager::setHomePosition(double lat, double lon, double alt)
{
    // Checking for NaN and infitiny
    // and checking for borders
    bool changed = false;
    if (!isnan(lat) && !isnan(lon) && !isnan(alt)
        && !isinf(lat) && !isinf(lon) && !isinf(alt)
        && lat <= 90.0 && lat >= -90.0 && lon <= 180.0 && lon >= -180.0)
        {

        if (homeLat != lat) changed = true;
        if (homeLon != lon) changed = true;
        if (homeAlt != alt) changed = true;

        if (changed)
        {
            homeLat = lat;
            homeLon = lon;
            homeAlt = alt;
            emit homePositionChanged(homeLat, homeLon, homeAlt);

            // Update all UAVs
            foreach (UASInterface* mav, systems)
            {
                mav->setHomePosition(homeLat, homeLon, homeAlt);
            }
        }
    }
    return changed;
}

/**
 * This function will change QGC's home position on a number of conditions only
 */
void UASManager::uavChangedHomePosition(int uav, double lat, double lon, double alt)
{
    // FIXME: Accept any home position change for now from the active UAS
    // this means that the currently select UAS can change the home location
    // of the whole swarm. This makes sense, but more control might be needed
    if (uav == activeUAS->getUASID())
    {
        if (setHomePosition(lat, lon, alt))
        {
            foreach (UASInterface* mav, systems)
            {
                // Only update the other systems, not the original source
                if (mav->getUASID() != uav)
                {
                    mav->setHomePosition(homeLat, homeLon, homeAlt);
                }
            }
        }
    }
}

/**
 * @brief Private singleton constructor
 *
 * This class implements the singleton design pattern and has therefore only a private constructor.
 **/
UASManager::UASManager() :
    activeUAS(NULL),
    homeLat(47.3769),
    homeLon(8.549444),
    homeAlt(470.0)
{
    start(QThread::LowPriority);
    loadSettings();
}

UASManager::~UASManager()
{
    storeSettings();
    // Delete all systems
    foreach (UASInterface* mav, systems) {
        delete mav;
    }
}


void UASManager::run()
{
//    forever
//    {
//        QGC::SLEEP::msleep(5000);
//    }
    exec();
}

void UASManager::addUAS(UASInterface* uas)
{
    // WARNING: The active uas is set here
    // and then announced below. This is necessary
    // to make sure the getActiveUAS() function
    // returns the UAS once the UASCreated() signal
    // is emitted. The code is thus NOT redundant.
    bool firstUAS = false;
    if (activeUAS == NULL) {
        firstUAS = true;
        activeUAS = uas;
    }

    // Only execute if there is no UAS at this index
    if (!systems.contains(uas)) {
        systems.append(uas);
        connect(uas, SIGNAL(destroyed(QObject*)), this, SLOT(removeUAS(QObject*)));
        // Set home position on UAV if set in UI
        // - this is done on a per-UAV basis
        // Set home position in UI if UAV chooses a new one (caution! if multiple UAVs are connected, take care!)
        connect(uas, SIGNAL(homePositionChanged(int,double,double,double)), this, SLOT(uavChangedHomePosition(int,double,double,double)));
        emit UASCreated(uas);
    }

    // If there is no active UAS yet, set the first one as the active UAS
    if (firstUAS) {
        setActiveUAS(uas);
    }
}

void UASManager::removeUAS(QObject* uas)
{
    UASInterface* mav = qobject_cast<UASInterface*>(uas);

    if (mav) {
        int listindex = systems.indexOf(mav);

        if (mav == activeUAS) {
            if (systems.count() > 1) {
                // We only set a new UAS if more than one is present
                if (listindex != 0) {
                    // The system to be removed is not at position 1
                    // set position one as new active system
                    setActiveUAS(systems.first());
                } else {
                    // The system to be removed is at position 1,
                    // select the next system
                    setActiveUAS(systems.at(1));
                }
            } else {
                // TODO send a null pointer if no UAS is present any more
                // This has to be proberly tested however, since it might
                // crash code parts not handling null pointers correctly.
            }
        }
        systems.removeAt(listindex);
    }
}

QList<UASInterface*> UASManager::getUASList()
{
    return systems;
}

UASInterface* UASManager::getActiveUAS()
{
    return activeUAS; ///< Return zero pointer if no UAS has been loaded
}

UASInterface* UASManager::silentGetActiveUAS()
{
    return activeUAS; ///< Return zero pointer if no UAS has been loaded
}

bool UASManager::launchActiveUAS()
{
    // If the active UAS is set, execute command
    if (getActiveUAS()) activeUAS->launch();
    return (activeUAS); ///< Returns true if the UAS exists, false else
}

bool UASManager::haltActiveUAS()
{
    // If the active UAS is set, execute command
    if (getActiveUAS()) activeUAS->halt();
    return (activeUAS); ///< Returns true if the UAS exists, false else
}

bool UASManager::continueActiveUAS()
{
    // If the active UAS is set, execute command
    if (getActiveUAS()) activeUAS->go();
    return (activeUAS); ///< Returns true if the UAS exists, false else
}

bool UASManager::returnActiveUAS()
{
    // If the active UAS is set, execute command
    if (getActiveUAS()) activeUAS->home();
    return (activeUAS); ///< Returns true if the UAS exists, false else
}

bool UASManager::stopActiveUAS()
{
    // If the active UAS is set, execute command
    if (getActiveUAS()) activeUAS->emergencySTOP();
    return (activeUAS); ///< Returns true if the UAS exists, false else
}

bool UASManager::killActiveUAS()
{
    if (getActiveUAS()) activeUAS->emergencyKILL();
    return (activeUAS);
}

bool UASManager::shutdownActiveUAS()
{
    if (getActiveUAS()) activeUAS->shutdown();
    return (activeUAS);
}

void UASManager::configureActiveUAS()
{
    UASInterface* actUAS = getActiveUAS();
    if(actUAS) {
        // Do something
    }
}

UASInterface* UASManager::getUASForId(int id)
{
    UASInterface* system = NULL;

    foreach(UASInterface* sys, systems) {
        if (sys->getUASID() == id) {
            system = sys;
        }
    }

    // Return NULL if not found
    return system;
}

void UASManager::setActiveUAS(UASInterface* uas)
{
    if (uas != NULL) {
        activeUASMutex.lock();
        if (activeUAS != NULL) {
            emit activeUASStatusChanged(activeUAS, false);
            emit activeUASStatusChanged(activeUAS->getUASID(), false);
        }
        activeUAS = uas;
        activeUASMutex.unlock();

        activeUAS->setSelected();
        emit activeUASSet(uas);
        emit activeUASSet(uas->getUASID());
        emit activeUASSetListIndex(systems.indexOf(uas));
        emit activeUASStatusChanged(uas, true);
        emit activeUASStatusChanged(uas->getUASID(), true);
    }
}

