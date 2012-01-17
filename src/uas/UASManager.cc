/*==================================================================
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

#define PI 3.1415926535897932384626433832795
#define MEAN_EARTH_DIAMETER	12756274.0
#define UMR	0.017453292519943295769236907684886

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

        // Initialize conversion reference in any case
        initReference(lat, lon, alt);

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
 * @param x1 Point 1 coordinate in x dimension
 * @param y1 Point 1 coordinate in y dimension
 * @param z1 Point 1 coordinate in z dimension
 *
 * @param x2 Point 2 coordinate in x dimension
 * @param y2 Point 2 coordinate in y dimension
 * @param z2 Point 2 coordinate in z dimension
 */
void UASManager::setLocalNEDSafetyBorders(double x1, double y1, double z1, double x2, double y2, double z2)
{
    nedSafetyLimitPosition1.x() = x1;
    nedSafetyLimitPosition1.y() = y1;
    nedSafetyLimitPosition1.z() = z1;

    nedSafetyLimitPosition2.x() = x2;
    nedSafetyLimitPosition2.y() = y2;
    nedSafetyLimitPosition2.z() = z2;
}


void UASManager::initReference(const double & latitude, const double & longitude, const double & altitude)
{
    Eigen::Matrix3d R;
    double s_long, s_lat, c_long, c_lat;
    sincos(latitude * DEG2RAD, &s_lat, &c_lat);
    sincos(longitude * DEG2RAD, &s_long, &c_long);

    R(0, 0) = -s_long;
    R(0, 1) = c_long;
    R(0, 2) = 0;

    R(1, 0) = -s_lat * c_long;
    R(1, 1) = -s_lat * s_long;
    R(1, 2) = c_lat;

    R(2, 0) = c_lat * c_long;
    R(2, 1) = c_lat * s_long;
    R(2, 2) = s_lat;

    ecef_ref_orientation_ = Eigen::Quaterniond(R);

    ecef_ref_point_ = wgs84ToEcef(latitude, longitude, altitude);
}

Eigen::Vector3d UASManager::wgs84ToEcef(const double & latitude, const double & longitude, const double & altitude)
{
    const double a = 6378137.0; // semi-major axis
    const double e_sq = 6.69437999014e-3; // first eccentricity squared

    double s_long, s_lat, c_long, c_lat;
    sincos(latitude * DEG2RAD, &s_lat, &c_lat);
    sincos(longitude * DEG2RAD, &s_long, &c_long);

    const double N = a / sqrt(1 - e_sq * s_lat * s_lat);

    Eigen::Vector3d ecef;

    ecef[0] = (N + altitude) * c_lat * c_long;
    ecef[1] = (N + altitude) * c_lat * s_long;
    ecef[2] = (N * (1 - e_sq) + altitude) * s_lat;

    return ecef;
}

Eigen::Vector3d UASManager::ecefToEnu(const Eigen::Vector3d & ecef)
{
    return ecef_ref_orientation_ * (ecef - ecef_ref_point_);
}

void UASManager::wgs84ToEnu(const double& lat, const double& lon, const double& alt, double* east, double* north, double* up)
{
    Eigen::Vector3d ecef = wgs84ToEcef(lat, lon, alt);
    Eigen::Vector3d enu = ecefToEnu(ecef);
    *east = enu.x();
    *north = enu.y();
    *up = enu.z();
}

//void UASManager::wgs84ToNed(const double& lat, const double& lon, const double& alt, double* north, double* east, double* down)
//{

//}



void UASManager::enuToWgs84(const double& x, const double& y, const double& z, double* lat, double* lon, double* alt)
{
    *lat=homeLat+y/MEAN_EARTH_DIAMETER*360./PI;
    *lon=homeLon+x/MEAN_EARTH_DIAMETER*360./PI/cos(homeLat*UMR);
    *alt=homeAlt+z;
}

void UASManager::nedToWgs84(const double& x, const double& y, const double& z, double* lat, double* lon, double* alt)
{
    *lat=homeLat+x/MEAN_EARTH_DIAMETER*360./PI;
    *lon=homeLon+y/MEAN_EARTH_DIAMETER*360./PI/cos(homeLat*UMR);
    *alt=homeAlt-z;
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
        homeAlt(470.0),
        homeFrame(MAV_FRAME_GLOBAL)
{
    loadSettings();
    setLocalNEDSafetyBorders(1, -1, 0, -1, 1, -1);
}

UASManager::~UASManager()
{
    storeSettings();
    // Delete all systems
    foreach (UASInterface* mav, systems) {
        delete mav;
    }
}

void UASManager::addUAS(UASInterface* uas)
{
    // WARNING: The active uas is set here
    // and then announced below. This is necessary
    // to make sure the getActiveUAS() function
    // returns the UAS once the UASCreated() signal
    // is emitted. The code is thus NOT redundant.
    bool firstUAS = false;
    if (activeUAS == NULL)
    {
        firstUAS = true;
        activeUAS = uas;
    }

    // Only execute if there is no UAS at this index
    if (!systems.contains(uas))
    {
        systems.append(uas);
        connect(uas, SIGNAL(destroyed(QObject*)), this, SLOT(removeUAS(QObject*)));
        // Set home position on UAV if set in UI
        // - this is done on a per-UAV basis
        // Set home position in UI if UAV chooses a new one (caution! if multiple UAVs are connected, take care!)
        connect(uas, SIGNAL(homePositionChanged(int,double,double,double)), this, SLOT(uavChangedHomePosition(int,double,double,double)));
        emit UASCreated(uas);
    }

    // If there is no active UAS yet, set the first one as the active UAS
    if (firstUAS)
    {
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

