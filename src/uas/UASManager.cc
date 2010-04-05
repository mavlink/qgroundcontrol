/*=====================================================================

PIXHAWK Micro Air Vehicle Flying Robotics Toolkit

(c) 2009 PIXHAWK PROJECT  <http://pixhawk.ethz.ch>

This file is part of the PIXHAWK project

    PIXHAWK is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    PIXHAWK is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with PIXHAWK. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief Implementation of central manager for all connected aerial vehicles
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <QList>
#include <QApplication>
#include <QMessageBox>
#include <QTimer>
#include "UAS.h"
#include <UASInterface.h>
#include <UASManager.h>

UASManager* UASManager::instance() {
    static UASManager* _instance = 0;
    if(_instance == 0) {
        _instance = new UASManager();

        // Set the application as parent to ensure that this object
        // will be destroyed when the main application exits
        _instance->setParent(qApp);
    }
    return _instance;
}

/**
 * @brief Private singleton constructor
 *
 * This class implements the singleton design pattern and has therefore only a private constructor.
 **/
UASManager::UASManager() :
        activeUAS(NULL)
{
    systems = QMap<int, UASInterface*>();
    start(QThread::LowPriority);
}

UASManager::~UASManager()
{
}


void UASManager::run()
{
}

void UASManager::addUAS(UASInterface* uas)
{
    // Only execute if there is no UAS at this index
    if (!systems.contains(uas->getUASID()))
    {
        systems.insert(uas->getUASID(), uas);
        emit UASCreated(uas);
    }

    // If there is no active UAS yet, set the first one as the active UAS
    if (activeUAS == NULL)
    {
        activeUAS = uas;
        emit activeUASSet(uas);
    }
}

UASInterface* UASManager::getActiveUAS()
{
    if(!activeUAS)
    {
        QMessageBox msgBox;
        msgBox.setText(tr("No Unmanned System loaded. Please add one first."));
        msgBox.exec();
    }
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
    if(actUAS)
    {
        // Do something
    }
}

UASInterface* UASManager::getUASForId(int id)
{
    // Return NULL pointer if UAS does not exist
    return systems.value(id, NULL);
}

void UASManager::setActiveUAS(UASInterface* uas)
{
    if (uas != NULL)
    {
        activeUASMutex.lock();
        activeUAS = uas;
        activeUASMutex.unlock();

        qDebug() << __FILE__ << ":" << __LINE__ << " ACTIVE UAS SET TO: " << uas->getUASName();

        emit activeUASSet(uas);
    }
}

