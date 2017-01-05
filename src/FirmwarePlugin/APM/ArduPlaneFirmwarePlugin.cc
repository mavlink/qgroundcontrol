/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Pritam Ghanghas <pritam.ghanghas@gmail.com>

#include "ArduPlaneFirmwarePlugin.h"

APMPlaneMode::APMPlaneMode(uint32_t mode, bool settable)
    : APMCustomMode(mode, settable)
{
    QMap<uint32_t,QString> enumToString;
    enumToString.insert(MANUAL,         "Manual");
    enumToString.insert(CIRCLE,         "Circle");
    enumToString.insert(STABILIZE,      "Stabilize");
    enumToString.insert(TRAINING,       "Training");
    enumToString.insert(ACRO,           "Acro");
    enumToString.insert(FLY_BY_WIRE_A,  "FWB A");
    enumToString.insert(FLY_BY_WIRE_B,  "FWB B");
    enumToString.insert(CRUISE,         "Cruise");
    enumToString.insert(AUTOTUNE,       "Autotune");
    enumToString.insert(AUTO,           "Auto");
    enumToString.insert(RTL,            "RTL");
    enumToString.insert(LOITER,         "Loiter");
    enumToString.insert(GUIDED,         "Guided");
    enumToString.insert(INITIALIZING,   "Initializing");
    enumToString.insert(QSTABILIZE,     "QuadPlane Stabilize");
    enumToString.insert(QHOVER,         "QuadPlane Hover");
    enumToString.insert(QLOITER,        "QuadPlane Loiter");
    enumToString.insert(QLAND,          "QuadPlane Land");
    enumToString.insert(QRTL,           "QuadPlane RTL");

    setEnumToStringMapping(enumToString);
}

ArduPlaneFirmwarePlugin::ArduPlaneFirmwarePlugin(void)
{
    QList<APMCustomMode> supportedFlightModes;
    supportedFlightModes << APMPlaneMode(APMPlaneMode::MANUAL          ,true);
    supportedFlightModes << APMPlaneMode(APMPlaneMode::CIRCLE          ,true);
    supportedFlightModes << APMPlaneMode(APMPlaneMode::STABILIZE       ,true);
    supportedFlightModes << APMPlaneMode(APMPlaneMode::TRAINING        ,true);
    supportedFlightModes << APMPlaneMode(APMPlaneMode::ACRO            ,true);
    supportedFlightModes << APMPlaneMode(APMPlaneMode::FLY_BY_WIRE_A   ,true);
    supportedFlightModes << APMPlaneMode(APMPlaneMode::FLY_BY_WIRE_B   ,true);
    supportedFlightModes << APMPlaneMode(APMPlaneMode::CRUISE          ,true);
    supportedFlightModes << APMPlaneMode(APMPlaneMode::AUTOTUNE        ,true);
    supportedFlightModes << APMPlaneMode(APMPlaneMode::AUTO            ,true);
    supportedFlightModes << APMPlaneMode(APMPlaneMode::RTL             ,true);
    supportedFlightModes << APMPlaneMode(APMPlaneMode::LOITER          ,true);
    supportedFlightModes << APMPlaneMode(APMPlaneMode::GUIDED          ,true);
    supportedFlightModes << APMPlaneMode(APMPlaneMode::INITIALIZING    ,false);
    supportedFlightModes << APMPlaneMode(APMPlaneMode::QSTABILIZE      ,true);
    supportedFlightModes << APMPlaneMode(APMPlaneMode::QHOVER          ,true);
    supportedFlightModes << APMPlaneMode(APMPlaneMode::QLOITER         ,true);
    supportedFlightModes << APMPlaneMode(APMPlaneMode::QLAND           ,true);
    supportedFlightModes << APMPlaneMode(APMPlaneMode::QRTL            ,true);
    setSupportedModes(supportedFlightModes);
}

QString ArduPlaneFirmwarePlugin::takeControlFlightMode(void)
{
    return QStringLiteral("Manual");
}
