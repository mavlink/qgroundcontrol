/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

#ifndef PX4ParameterFacts_h
#define PX4ParameterFacts_h

#include <QObject>
#include <QMap>
#include <QXmlStreamReader>
#include <QLoggingCategory>

#include "Fact.h"
#include "UASInterface.h"

/// @file
///     @author Don Gagne <don@thegagnes.com>

// FIXME: This file should be auto-generated from the Parameter XML file.

Q_DECLARE_LOGGING_CATEGORY(PX4ParameterFactsLog)
Q_DECLARE_LOGGING_CATEGORY(PX4ParameterFactsMetaDataLog)

/// Collection of Parameter Facts for PX4 AutoPilot
///
/// These Facts are available for binding within QML code. For example:
/// @code{.unparsed}
///     TextInput {
///         text: parameterFacts.RC_MAP_THROTTLE.value
///     }
/// @endcode

class PX4ParameterFacts : public QObject
{
    Q_OBJECT
    
    Q_PROPERTY(Fact* ATT_ACC_COMP READ getATT_ACC_COMP CONSTANT) Fact* getATT_ACC_COMP(void) { return _mapParameterName2Fact["ATT_ACC_COMP"]; }
    Q_PROPERTY(Fact* ATT_MAG_DECL READ getATT_MAG_DECL CONSTANT) Fact* getATT_MAG_DECL(void) { return _mapParameterName2Fact["ATT_MAG_DECL"]; }
    Q_PROPERTY(Fact* BAT_CAPACITY READ getBAT_CAPACITY CONSTANT) Fact* getBAT_CAPACITY(void) { return _mapParameterName2Fact["BAT_CAPACITY"]; }
    Q_PROPERTY(Fact* BAT_C_SCALING READ getBAT_C_SCALING CONSTANT) Fact* getBAT_C_SCALING(void) { return _mapParameterName2Fact["BAT_C_SCALING"]; }
    Q_PROPERTY(Fact* BAT_N_CELLS READ getBAT_N_CELLS CONSTANT) Fact* getBAT_N_CELLS(void) { return _mapParameterName2Fact["BAT_N_CELLS"]; }
    Q_PROPERTY(Fact* BAT_V_CHARGED READ getBAT_V_CHARGED CONSTANT) Fact* getBAT_V_CHARGED(void) { return _mapParameterName2Fact["BAT_V_CHARGED"]; }
    Q_PROPERTY(Fact* BAT_V_EMPTY READ getBAT_V_EMPTY CONSTANT) Fact* getBAT_V_EMPTY(void) { return _mapParameterName2Fact["BAT_V_EMPTY"]; }
    Q_PROPERTY(Fact* BAT_V_LOAD_DROP READ getBAT_V_LOAD_DROP CONSTANT) Fact* getBAT_V_LOAD_DROP(void) { return _mapParameterName2Fact["BAT_V_LOAD_DROP"]; }
    Q_PROPERTY(Fact* BAT_V_SCALE_IO READ getBAT_V_SCALE_IO CONSTANT) Fact* getBAT_V_SCALE_IO(void) { return _mapParameterName2Fact["BAT_V_SCALE_IO"]; }
    Q_PROPERTY(Fact* BAT_V_SCALING READ getBAT_V_SCALING CONSTANT) Fact* getBAT_V_SCALING(void) { return _mapParameterName2Fact["BAT_V_SCALING"]; }
    Q_PROPERTY(Fact* BD_GPROPERTIES READ getBD_GPROPERTIES CONSTANT) Fact* getBD_GPROPERTIES(void) { return _mapParameterName2Fact["BD_GPROPERTIES"]; }
    Q_PROPERTY(Fact* BD_OBJ_CD READ getBD_OBJ_CD CONSTANT) Fact* getBD_OBJ_CD(void) { return _mapParameterName2Fact["BD_OBJ_CD"]; }
    Q_PROPERTY(Fact* BD_OBJ_MASS READ getBD_OBJ_MASS CONSTANT) Fact* getBD_OBJ_MASS(void) { return _mapParameterName2Fact["BD_OBJ_MASS"]; }
    Q_PROPERTY(Fact* BD_OBJ_SURFACE READ getBD_OBJ_SURFACE CONSTANT) Fact* getBD_OBJ_SURFACE(void) { return _mapParameterName2Fact["BD_OBJ_SURFACE"]; }
    Q_PROPERTY(Fact* BD_PRECISION READ getBD_PRECISION CONSTANT) Fact* getBD_PRECISION(void) { return _mapParameterName2Fact["BD_PRECISION"]; }
    Q_PROPERTY(Fact* BD_TURNRADIUS READ getBD_TURNRADIUS CONSTANT) Fact* getBD_TURNRADIUS(void) { return _mapParameterName2Fact["BD_TURNRADIUS"]; }
    Q_PROPERTY(Fact* CBRK_AIRSPD_CHK READ getCBRK_AIRSPD_CHK CONSTANT) Fact* getCBRK_AIRSPD_CHK(void) { return _mapParameterName2Fact["CBRK_AIRSPD_CHK"]; }
    Q_PROPERTY(Fact* CBRK_ENGINEFAIL READ getCBRK_ENGINEFAIL CONSTANT) Fact* getCBRK_ENGINEFAIL(void) { return _mapParameterName2Fact["CBRK_ENGINEFAIL"]; }
    Q_PROPERTY(Fact* CBRK_FLIGHTTERM READ getCBRK_FLIGHTTERM CONSTANT) Fact* getCBRK_FLIGHTTERM(void) { return _mapParameterName2Fact["CBRK_FLIGHTTERM"]; }
    Q_PROPERTY(Fact* CBRK_GPSFAIL READ getCBRK_GPSFAIL CONSTANT) Fact* getCBRK_GPSFAIL(void) { return _mapParameterName2Fact["CBRK_GPSFAIL"]; }
    Q_PROPERTY(Fact* CBRK_IO_SAFETY READ getCBRK_IO_SAFETY CONSTANT) Fact* getCBRK_IO_SAFETY(void) { return _mapParameterName2Fact["CBRK_IO_SAFETY"]; }
    Q_PROPERTY(Fact* CBRK_NO_VISION READ getCBRK_NO_VISION CONSTANT) Fact* getCBRK_NO_VISION(void) { return _mapParameterName2Fact["CBRK_NO_VISION"]; }
    Q_PROPERTY(Fact* CBRK_RATE_CTRL READ getCBRK_RATE_CTRL CONSTANT) Fact* getCBRK_RATE_CTRL(void) { return _mapParameterName2Fact["CBRK_RATE_CTRL"]; }
    Q_PROPERTY(Fact* CBRK_SUPPLY_CHK READ getCBRK_SUPPLY_CHK CONSTANT) Fact* getCBRK_SUPPLY_CHK(void) { return _mapParameterName2Fact["CBRK_SUPPLY_CHK"]; }
    Q_PROPERTY(Fact* COM_DL_LOSS_EN READ getCOM_DL_LOSS_EN CONSTANT) Fact* getCOM_DL_LOSS_EN(void) { return _mapParameterName2Fact["COM_DL_LOSS_EN"]; }
    Q_PROPERTY(Fact* COM_DL_LOSS_T READ getCOM_DL_LOSS_T CONSTANT) Fact* getCOM_DL_LOSS_T(void) { return _mapParameterName2Fact["COM_DL_LOSS_T"]; }
    Q_PROPERTY(Fact* COM_DL_REG_T READ getCOM_DL_REG_T CONSTANT) Fact* getCOM_DL_REG_T(void) { return _mapParameterName2Fact["COM_DL_REG_T"]; }
    Q_PROPERTY(Fact* COM_EF_C2T READ getCOM_EF_C2T CONSTANT) Fact* getCOM_EF_C2T(void) { return _mapParameterName2Fact["COM_EF_C2T"]; }
    Q_PROPERTY(Fact* COM_EF_THROT READ getCOM_EF_THROT CONSTANT) Fact* getCOM_EF_THROT(void) { return _mapParameterName2Fact["COM_EF_THROT"]; }
    Q_PROPERTY(Fact* COM_EF_TIME READ getCOM_EF_TIME CONSTANT) Fact* getCOM_EF_TIME(void) { return _mapParameterName2Fact["COM_EF_TIME"]; }
    Q_PROPERTY(Fact* COM_RC_LOSS_T READ getCOM_RC_LOSS_T CONSTANT) Fact* getCOM_RC_LOSS_T(void) { return _mapParameterName2Fact["COM_RC_LOSS_T"]; }
    Q_PROPERTY(Fact* EKF_ATT_V3_Q0 READ getEKF_ATT_V3_Q0 CONSTANT) Fact* getEKF_ATT_V3_Q0(void) { return _mapParameterName2Fact["EKF_ATT_V3_Q0"]; }
    Q_PROPERTY(Fact* EKF_ATT_V3_Q1 READ getEKF_ATT_V3_Q1 CONSTANT) Fact* getEKF_ATT_V3_Q1(void) { return _mapParameterName2Fact["EKF_ATT_V3_Q1"]; }
    Q_PROPERTY(Fact* EKF_ATT_V3_Q2 READ getEKF_ATT_V3_Q2 CONSTANT) Fact* getEKF_ATT_V3_Q2(void) { return _mapParameterName2Fact["EKF_ATT_V3_Q2"]; }
    Q_PROPERTY(Fact* EKF_ATT_V3_Q3 READ getEKF_ATT_V3_Q3 CONSTANT) Fact* getEKF_ATT_V3_Q3(void) { return _mapParameterName2Fact["EKF_ATT_V3_Q3"]; }
    Q_PROPERTY(Fact* EKF_ATT_V3_Q4 READ getEKF_ATT_V3_Q4 CONSTANT) Fact* getEKF_ATT_V3_Q4(void) { return _mapParameterName2Fact["EKF_ATT_V3_Q4"]; }
    Q_PROPERTY(Fact* EKF_ATT_V4_R0 READ getEKF_ATT_V4_R0 CONSTANT) Fact* getEKF_ATT_V4_R0(void) { return _mapParameterName2Fact["EKF_ATT_V4_R0"]; }
    Q_PROPERTY(Fact* EKF_ATT_V4_R1 READ getEKF_ATT_V4_R1 CONSTANT) Fact* getEKF_ATT_V4_R1(void) { return _mapParameterName2Fact["EKF_ATT_V4_R1"]; }
    Q_PROPERTY(Fact* EKF_ATT_V4_R2 READ getEKF_ATT_V4_R2 CONSTANT) Fact* getEKF_ATT_V4_R2(void) { return _mapParameterName2Fact["EKF_ATT_V4_R2"]; }
    Q_PROPERTY(Fact* EKF_ATT_V4_R3 READ getEKF_ATT_V4_R3 CONSTANT) Fact* getEKF_ATT_V4_R3(void) { return _mapParameterName2Fact["EKF_ATT_V4_R3"]; }
    Q_PROPERTY(Fact* FPE_DEBUG READ getFPE_DEBUG CONSTANT) Fact* getFPE_DEBUG(void) { return _mapParameterName2Fact["FPE_DEBUG"]; }
    Q_PROPERTY(Fact* FPE_LO_THRUST READ getFPE_LO_THRUST CONSTANT) Fact* getFPE_LO_THRUST(void) { return _mapParameterName2Fact["FPE_LO_THRUST"]; }
    Q_PROPERTY(Fact* FPE_SONAR_LP_L READ getFPE_SONAR_LP_L CONSTANT) Fact* getFPE_SONAR_LP_L(void) { return _mapParameterName2Fact["FPE_SONAR_LP_L"]; }
    Q_PROPERTY(Fact* FPE_SONAR_LP_U READ getFPE_SONAR_LP_U CONSTANT) Fact* getFPE_SONAR_LP_U(void) { return _mapParameterName2Fact["FPE_SONAR_LP_U"]; }
    Q_PROPERTY(Fact* FW_AIRSPD_MAX READ getFW_AIRSPD_MAX CONSTANT) Fact* getFW_AIRSPD_MAX(void) { return _mapParameterName2Fact["FW_AIRSPD_MAX"]; }
    Q_PROPERTY(Fact* FW_AIRSPD_MIN READ getFW_AIRSPD_MIN CONSTANT) Fact* getFW_AIRSPD_MIN(void) { return _mapParameterName2Fact["FW_AIRSPD_MIN"]; }
    Q_PROPERTY(Fact* FW_AIRSPD_TRIM READ getFW_AIRSPD_TRIM CONSTANT) Fact* getFW_AIRSPD_TRIM(void) { return _mapParameterName2Fact["FW_AIRSPD_TRIM"]; }
    Q_PROPERTY(Fact* FW_ATT_TC READ getFW_ATT_TC CONSTANT) Fact* getFW_ATT_TC(void) { return _mapParameterName2Fact["FW_ATT_TC"]; }
    Q_PROPERTY(Fact* FW_CLMBOUT_DIFF READ getFW_CLMBOUT_DIFF CONSTANT) Fact* getFW_CLMBOUT_DIFF(void) { return _mapParameterName2Fact["FW_CLMBOUT_DIFF"]; }
    Q_PROPERTY(Fact* FW_FLARE_PMAX READ getFW_FLARE_PMAX CONSTANT) Fact* getFW_FLARE_PMAX(void) { return _mapParameterName2Fact["FW_FLARE_PMAX"]; }
    Q_PROPERTY(Fact* FW_FLARE_PMIN READ getFW_FLARE_PMIN CONSTANT) Fact* getFW_FLARE_PMIN(void) { return _mapParameterName2Fact["FW_FLARE_PMIN"]; }
    Q_PROPERTY(Fact* FW_L1_DAMPING READ getFW_L1_DAMPING CONSTANT) Fact* getFW_L1_DAMPING(void) { return _mapParameterName2Fact["FW_L1_DAMPING"]; }
    Q_PROPERTY(Fact* FW_L1_PERIOD READ getFW_L1_PERIOD CONSTANT) Fact* getFW_L1_PERIOD(void) { return _mapParameterName2Fact["FW_L1_PERIOD"]; }
    Q_PROPERTY(Fact* FW_LND_ANG READ getFW_LND_ANG CONSTANT) Fact* getFW_LND_ANG(void) { return _mapParameterName2Fact["FW_LND_ANG"]; }
    Q_PROPERTY(Fact* FW_LND_FLALT READ getFW_LND_FLALT CONSTANT) Fact* getFW_LND_FLALT(void) { return _mapParameterName2Fact["FW_LND_FLALT"]; }
    Q_PROPERTY(Fact* FW_LND_HHDIST READ getFW_LND_HHDIST CONSTANT) Fact* getFW_LND_HHDIST(void) { return _mapParameterName2Fact["FW_LND_HHDIST"]; }
    Q_PROPERTY(Fact* FW_LND_HVIRT READ getFW_LND_HVIRT CONSTANT) Fact* getFW_LND_HVIRT(void) { return _mapParameterName2Fact["FW_LND_HVIRT"]; }
    Q_PROPERTY(Fact* FW_LND_TLALT READ getFW_LND_TLALT CONSTANT) Fact* getFW_LND_TLALT(void) { return _mapParameterName2Fact["FW_LND_TLALT"]; }
    Q_PROPERTY(Fact* FW_LND_USETER READ getFW_LND_USETER CONSTANT) Fact* getFW_LND_USETER(void) { return _mapParameterName2Fact["FW_LND_USETER"]; }
    Q_PROPERTY(Fact* FW_MAN_P_MAX READ getFW_MAN_P_MAX CONSTANT) Fact* getFW_MAN_P_MAX(void) { return _mapParameterName2Fact["FW_MAN_P_MAX"]; }
    Q_PROPERTY(Fact* FW_MAN_R_MAX READ getFW_MAN_R_MAX CONSTANT) Fact* getFW_MAN_R_MAX(void) { return _mapParameterName2Fact["FW_MAN_R_MAX"]; }
    Q_PROPERTY(Fact* FW_PR_FF READ getFW_PR_FF CONSTANT) Fact* getFW_PR_FF(void) { return _mapParameterName2Fact["FW_PR_FF"]; }
    Q_PROPERTY(Fact* FW_PR_I READ getFW_PR_I CONSTANT) Fact* getFW_PR_I(void) { return _mapParameterName2Fact["FW_PR_I"]; }
    Q_PROPERTY(Fact* FW_PR_IMAX READ getFW_PR_IMAX CONSTANT) Fact* getFW_PR_IMAX(void) { return _mapParameterName2Fact["FW_PR_IMAX"]; }
    Q_PROPERTY(Fact* FW_PR_P READ getFW_PR_P CONSTANT) Fact* getFW_PR_P(void) { return _mapParameterName2Fact["FW_PR_P"]; }
    Q_PROPERTY(Fact* FW_PSP_OFF READ getFW_PSP_OFF CONSTANT) Fact* getFW_PSP_OFF(void) { return _mapParameterName2Fact["FW_PSP_OFF"]; }
    Q_PROPERTY(Fact* FW_P_LIM_MAX READ getFW_P_LIM_MAX CONSTANT) Fact* getFW_P_LIM_MAX(void) { return _mapParameterName2Fact["FW_P_LIM_MAX"]; }
    Q_PROPERTY(Fact* FW_P_LIM_MIN READ getFW_P_LIM_MIN CONSTANT) Fact* getFW_P_LIM_MIN(void) { return _mapParameterName2Fact["FW_P_LIM_MIN"]; }
    Q_PROPERTY(Fact* FW_P_RMAX_NEG READ getFW_P_RMAX_NEG CONSTANT) Fact* getFW_P_RMAX_NEG(void) { return _mapParameterName2Fact["FW_P_RMAX_NEG"]; }
    Q_PROPERTY(Fact* FW_P_RMAX_POS READ getFW_P_RMAX_POS CONSTANT) Fact* getFW_P_RMAX_POS(void) { return _mapParameterName2Fact["FW_P_RMAX_POS"]; }
    Q_PROPERTY(Fact* FW_P_ROLLFF READ getFW_P_ROLLFF CONSTANT) Fact* getFW_P_ROLLFF(void) { return _mapParameterName2Fact["FW_P_ROLLFF"]; }
    Q_PROPERTY(Fact* FW_RR_FF READ getFW_RR_FF CONSTANT) Fact* getFW_RR_FF(void) { return _mapParameterName2Fact["FW_RR_FF"]; }
    Q_PROPERTY(Fact* FW_RR_I READ getFW_RR_I CONSTANT) Fact* getFW_RR_I(void) { return _mapParameterName2Fact["FW_RR_I"]; }
    Q_PROPERTY(Fact* FW_RR_IMAX READ getFW_RR_IMAX CONSTANT) Fact* getFW_RR_IMAX(void) { return _mapParameterName2Fact["FW_RR_IMAX"]; }
    Q_PROPERTY(Fact* FW_RR_P READ getFW_RR_P CONSTANT) Fact* getFW_RR_P(void) { return _mapParameterName2Fact["FW_RR_P"]; }
    Q_PROPERTY(Fact* FW_RSP_OFF READ getFW_RSP_OFF CONSTANT) Fact* getFW_RSP_OFF(void) { return _mapParameterName2Fact["FW_RSP_OFF"]; }
    Q_PROPERTY(Fact* FW_R_LIM READ getFW_R_LIM CONSTANT) Fact* getFW_R_LIM(void) { return _mapParameterName2Fact["FW_R_LIM"]; }
    Q_PROPERTY(Fact* FW_R_RMAX READ getFW_R_RMAX CONSTANT) Fact* getFW_R_RMAX(void) { return _mapParameterName2Fact["FW_R_RMAX"]; }
    Q_PROPERTY(Fact* FW_THR_CRUISE READ getFW_THR_CRUISE CONSTANT) Fact* getFW_THR_CRUISE(void) { return _mapParameterName2Fact["FW_THR_CRUISE"]; }
    Q_PROPERTY(Fact* FW_THR_LND_MAX READ getFW_THR_LND_MAX CONSTANT) Fact* getFW_THR_LND_MAX(void) { return _mapParameterName2Fact["FW_THR_LND_MAX"]; }
    Q_PROPERTY(Fact* FW_THR_MAX READ getFW_THR_MAX CONSTANT) Fact* getFW_THR_MAX(void) { return _mapParameterName2Fact["FW_THR_MAX"]; }
    Q_PROPERTY(Fact* FW_THR_MIN READ getFW_THR_MIN CONSTANT) Fact* getFW_THR_MIN(void) { return _mapParameterName2Fact["FW_THR_MIN"]; }
    Q_PROPERTY(Fact* FW_THR_SLEW_MAX READ getFW_THR_SLEW_MAX CONSTANT) Fact* getFW_THR_SLEW_MAX(void) { return _mapParameterName2Fact["FW_THR_SLEW_MAX"]; }
    Q_PROPERTY(Fact* FW_T_CLMB_MAX READ getFW_T_CLMB_MAX CONSTANT) Fact* getFW_T_CLMB_MAX(void) { return _mapParameterName2Fact["FW_T_CLMB_MAX"]; }
    Q_PROPERTY(Fact* FW_T_HGT_OMEGA READ getFW_T_HGT_OMEGA CONSTANT) Fact* getFW_T_HGT_OMEGA(void) { return _mapParameterName2Fact["FW_T_HGT_OMEGA"]; }
    Q_PROPERTY(Fact* FW_T_HRATE_FF READ getFW_T_HRATE_FF CONSTANT) Fact* getFW_T_HRATE_FF(void) { return _mapParameterName2Fact["FW_T_HRATE_FF"]; }
    Q_PROPERTY(Fact* FW_T_HRATE_P READ getFW_T_HRATE_P CONSTANT) Fact* getFW_T_HRATE_P(void) { return _mapParameterName2Fact["FW_T_HRATE_P"]; }
    Q_PROPERTY(Fact* FW_T_INTEG_GAIN READ getFW_T_INTEG_GAIN CONSTANT) Fact* getFW_T_INTEG_GAIN(void) { return _mapParameterName2Fact["FW_T_INTEG_GAIN"]; }
    Q_PROPERTY(Fact* FW_T_PTCH_DAMP READ getFW_T_PTCH_DAMP CONSTANT) Fact* getFW_T_PTCH_DAMP(void) { return _mapParameterName2Fact["FW_T_PTCH_DAMP"]; }
    Q_PROPERTY(Fact* FW_T_RLL2THR READ getFW_T_RLL2THR CONSTANT) Fact* getFW_T_RLL2THR(void) { return _mapParameterName2Fact["FW_T_RLL2THR"]; }
    Q_PROPERTY(Fact* FW_T_SINK_MAX READ getFW_T_SINK_MAX CONSTANT) Fact* getFW_T_SINK_MAX(void) { return _mapParameterName2Fact["FW_T_SINK_MAX"]; }
    Q_PROPERTY(Fact* FW_T_SINK_MIN READ getFW_T_SINK_MIN CONSTANT) Fact* getFW_T_SINK_MIN(void) { return _mapParameterName2Fact["FW_T_SINK_MIN"]; }
    Q_PROPERTY(Fact* FW_T_SPDWEIGHT READ getFW_T_SPDWEIGHT CONSTANT) Fact* getFW_T_SPDWEIGHT(void) { return _mapParameterName2Fact["FW_T_SPDWEIGHT"]; }
    Q_PROPERTY(Fact* FW_T_SPD_OMEGA READ getFW_T_SPD_OMEGA CONSTANT) Fact* getFW_T_SPD_OMEGA(void) { return _mapParameterName2Fact["FW_T_SPD_OMEGA"]; }
    Q_PROPERTY(Fact* FW_T_SRATE_P READ getFW_T_SRATE_P CONSTANT) Fact* getFW_T_SRATE_P(void) { return _mapParameterName2Fact["FW_T_SRATE_P"]; }
    Q_PROPERTY(Fact* FW_T_THRO_CONST READ getFW_T_THRO_CONST CONSTANT) Fact* getFW_T_THRO_CONST(void) { return _mapParameterName2Fact["FW_T_THRO_CONST"]; }
    Q_PROPERTY(Fact* FW_T_THR_DAMP READ getFW_T_THR_DAMP CONSTANT) Fact* getFW_T_THR_DAMP(void) { return _mapParameterName2Fact["FW_T_THR_DAMP"]; }
    Q_PROPERTY(Fact* FW_T_TIME_CONST READ getFW_T_TIME_CONST CONSTANT) Fact* getFW_T_TIME_CONST(void) { return _mapParameterName2Fact["FW_T_TIME_CONST"]; }
    Q_PROPERTY(Fact* FW_T_VERT_ACC READ getFW_T_VERT_ACC CONSTANT) Fact* getFW_T_VERT_ACC(void) { return _mapParameterName2Fact["FW_T_VERT_ACC"]; }
    Q_PROPERTY(Fact* FW_YCO_VMIN READ getFW_YCO_VMIN CONSTANT) Fact* getFW_YCO_VMIN(void) { return _mapParameterName2Fact["FW_YCO_VMIN"]; }
    Q_PROPERTY(Fact* FW_YR_FF READ getFW_YR_FF CONSTANT) Fact* getFW_YR_FF(void) { return _mapParameterName2Fact["FW_YR_FF"]; }
    Q_PROPERTY(Fact* FW_YR_I READ getFW_YR_I CONSTANT) Fact* getFW_YR_I(void) { return _mapParameterName2Fact["FW_YR_I"]; }
    Q_PROPERTY(Fact* FW_YR_IMAX READ getFW_YR_IMAX CONSTANT) Fact* getFW_YR_IMAX(void) { return _mapParameterName2Fact["FW_YR_IMAX"]; }
    Q_PROPERTY(Fact* FW_YR_P READ getFW_YR_P CONSTANT) Fact* getFW_YR_P(void) { return _mapParameterName2Fact["FW_YR_P"]; }
    Q_PROPERTY(Fact* FW_Y_RMAX READ getFW_Y_RMAX CONSTANT) Fact* getFW_Y_RMAX(void) { return _mapParameterName2Fact["FW_Y_RMAX"]; }
    Q_PROPERTY(Fact* GF_ALTMODE READ getGF_ALTMODE CONSTANT) Fact* getGF_ALTMODE(void) { return _mapParameterName2Fact["GF_ALTMODE"]; }
    Q_PROPERTY(Fact* GF_COUNT READ getGF_COUNT CONSTANT) Fact* getGF_COUNT(void) { return _mapParameterName2Fact["GF_COUNT"]; }
    Q_PROPERTY(Fact* GF_ON READ getGF_ON CONSTANT) Fact* getGF_ON(void) { return _mapParameterName2Fact["GF_ON"]; }
    Q_PROPERTY(Fact* GF_SOURCE READ getGF_SOURCE CONSTANT) Fact* getGF_SOURCE(void) { return _mapParameterName2Fact["GF_SOURCE"]; }
    Q_PROPERTY(Fact* INAV_DELAY_GPS READ getINAV_DELAY_GPS CONSTANT) Fact* getINAV_DELAY_GPS(void) { return _mapParameterName2Fact["INAV_DELAY_GPS"]; }
    Q_PROPERTY(Fact* INAV_FLOW_K READ getINAV_FLOW_K CONSTANT) Fact* getINAV_FLOW_K(void) { return _mapParameterName2Fact["INAV_FLOW_K"]; }
    Q_PROPERTY(Fact* INAV_FLOW_Q_MIN READ getINAV_FLOW_Q_MIN CONSTANT) Fact* getINAV_FLOW_Q_MIN(void) { return _mapParameterName2Fact["INAV_FLOW_Q_MIN"]; }
    Q_PROPERTY(Fact* INAV_LAND_DISP READ getINAV_LAND_DISP CONSTANT) Fact* getINAV_LAND_DISP(void) { return _mapParameterName2Fact["INAV_LAND_DISP"]; }
    Q_PROPERTY(Fact* INAV_LAND_T READ getINAV_LAND_T CONSTANT) Fact* getINAV_LAND_T(void) { return _mapParameterName2Fact["INAV_LAND_T"]; }
    Q_PROPERTY(Fact* INAV_LAND_THR READ getINAV_LAND_THR CONSTANT) Fact* getINAV_LAND_THR(void) { return _mapParameterName2Fact["INAV_LAND_THR"]; }
    Q_PROPERTY(Fact* INAV_SONAR_ERR READ getINAV_SONAR_ERR CONSTANT) Fact* getINAV_SONAR_ERR(void) { return _mapParameterName2Fact["INAV_SONAR_ERR"]; }
    Q_PROPERTY(Fact* INAV_SONAR_FILT READ getINAV_SONAR_FILT CONSTANT) Fact* getINAV_SONAR_FILT(void) { return _mapParameterName2Fact["INAV_SONAR_FILT"]; }
    Q_PROPERTY(Fact* INAV_W_ACC_BIAS READ getINAV_W_ACC_BIAS CONSTANT) Fact* getINAV_W_ACC_BIAS(void) { return _mapParameterName2Fact["INAV_W_ACC_BIAS"]; }
    Q_PROPERTY(Fact* INAV_W_GPS_FLOW READ getINAV_W_GPS_FLOW CONSTANT) Fact* getINAV_W_GPS_FLOW(void) { return _mapParameterName2Fact["INAV_W_GPS_FLOW"]; }
    Q_PROPERTY(Fact* INAV_W_XY_FLOW READ getINAV_W_XY_FLOW CONSTANT) Fact* getINAV_W_XY_FLOW(void) { return _mapParameterName2Fact["INAV_W_XY_FLOW"]; }
    Q_PROPERTY(Fact* INAV_W_XY_GPS_P READ getINAV_W_XY_GPS_P CONSTANT) Fact* getINAV_W_XY_GPS_P(void) { return _mapParameterName2Fact["INAV_W_XY_GPS_P"]; }
    Q_PROPERTY(Fact* INAV_W_XY_GPS_V READ getINAV_W_XY_GPS_V CONSTANT) Fact* getINAV_W_XY_GPS_V(void) { return _mapParameterName2Fact["INAV_W_XY_GPS_V"]; }
    Q_PROPERTY(Fact* INAV_W_XY_RES_V READ getINAV_W_XY_RES_V CONSTANT) Fact* getINAV_W_XY_RES_V(void) { return _mapParameterName2Fact["INAV_W_XY_RES_V"]; }
    Q_PROPERTY(Fact* INAV_W_XY_VIS_P READ getINAV_W_XY_VIS_P CONSTANT) Fact* getINAV_W_XY_VIS_P(void) { return _mapParameterName2Fact["INAV_W_XY_VIS_P"]; }
    Q_PROPERTY(Fact* INAV_W_XY_VIS_V READ getINAV_W_XY_VIS_V CONSTANT) Fact* getINAV_W_XY_VIS_V(void) { return _mapParameterName2Fact["INAV_W_XY_VIS_V"]; }
    Q_PROPERTY(Fact* INAV_W_Z_BARO READ getINAV_W_Z_BARO CONSTANT) Fact* getINAV_W_Z_BARO(void) { return _mapParameterName2Fact["INAV_W_Z_BARO"]; }
    Q_PROPERTY(Fact* INAV_W_Z_GPS_P READ getINAV_W_Z_GPS_P CONSTANT) Fact* getINAV_W_Z_GPS_P(void) { return _mapParameterName2Fact["INAV_W_Z_GPS_P"]; }
    Q_PROPERTY(Fact* INAV_W_Z_SONAR READ getINAV_W_Z_SONAR CONSTANT) Fact* getINAV_W_Z_SONAR(void) { return _mapParameterName2Fact["INAV_W_Z_SONAR"]; }
    Q_PROPERTY(Fact* INAV_W_Z_VIS_P READ getINAV_W_Z_VIS_P CONSTANT) Fact* getINAV_W_Z_VIS_P(void) { return _mapParameterName2Fact["INAV_W_Z_VIS_P"]; }
    Q_PROPERTY(Fact* LAUN_ALL_ON READ getLAUN_ALL_ON CONSTANT) Fact* getLAUN_ALL_ON(void) { return _mapParameterName2Fact["LAUN_ALL_ON"]; }
    Q_PROPERTY(Fact* LAUN_CAT_A READ getLAUN_CAT_A CONSTANT) Fact* getLAUN_CAT_A(void) { return _mapParameterName2Fact["LAUN_CAT_A"]; }
    Q_PROPERTY(Fact* LAUN_CAT_MDEL READ getLAUN_CAT_MDEL CONSTANT) Fact* getLAUN_CAT_MDEL(void) { return _mapParameterName2Fact["LAUN_CAT_MDEL"]; }
    Q_PROPERTY(Fact* LAUN_CAT_PMAX READ getLAUN_CAT_PMAX CONSTANT) Fact* getLAUN_CAT_PMAX(void) { return _mapParameterName2Fact["LAUN_CAT_PMAX"]; }
    Q_PROPERTY(Fact* LAUN_CAT_T READ getLAUN_CAT_T CONSTANT) Fact* getLAUN_CAT_T(void) { return _mapParameterName2Fact["LAUN_CAT_T"]; }
    Q_PROPERTY(Fact* LAUN_THR_PRE READ getLAUN_THR_PRE CONSTANT) Fact* getLAUN_THR_PRE(void) { return _mapParameterName2Fact["LAUN_THR_PRE"]; }
    Q_PROPERTY(Fact* MAV_COMP_ID READ getMAV_COMP_ID CONSTANT) Fact* getMAV_COMP_ID(void) { return _mapParameterName2Fact["MAV_COMP_ID"]; }
    Q_PROPERTY(Fact* MAV_FWDEXTSP READ getMAV_FWDEXTSP CONSTANT) Fact* getMAV_FWDEXTSP(void) { return _mapParameterName2Fact["MAV_FWDEXTSP"]; }
    Q_PROPERTY(Fact* MAV_SYS_ID READ getMAV_SYS_ID CONSTANT) Fact* getMAV_SYS_ID(void) { return _mapParameterName2Fact["MAV_SYS_ID"]; }
    Q_PROPERTY(Fact* MAV_TYPE READ getMAV_TYPE CONSTANT) Fact* getMAV_TYPE(void) { return _mapParameterName2Fact["MAV_TYPE"]; }
    Q_PROPERTY(Fact* MAV_USEHILGPS READ getMAV_USEHILGPS CONSTANT) Fact* getMAV_USEHILGPS(void) { return _mapParameterName2Fact["MAV_USEHILGPS"]; }
    Q_PROPERTY(Fact* MC_ACRO_P_MAX READ getMC_ACRO_P_MAX CONSTANT) Fact* getMC_ACRO_P_MAX(void) { return _mapParameterName2Fact["MC_ACRO_P_MAX"]; }
    Q_PROPERTY(Fact* MC_ACRO_R_MAX READ getMC_ACRO_R_MAX CONSTANT) Fact* getMC_ACRO_R_MAX(void) { return _mapParameterName2Fact["MC_ACRO_R_MAX"]; }
    Q_PROPERTY(Fact* MC_ACRO_Y_MAX READ getMC_ACRO_Y_MAX CONSTANT) Fact* getMC_ACRO_Y_MAX(void) { return _mapParameterName2Fact["MC_ACRO_Y_MAX"]; }
    Q_PROPERTY(Fact* MC_MAN_P_MAX READ getMC_MAN_P_MAX CONSTANT) Fact* getMC_MAN_P_MAX(void) { return _mapParameterName2Fact["MC_MAN_P_MAX"]; }
    Q_PROPERTY(Fact* MC_MAN_R_MAX READ getMC_MAN_R_MAX CONSTANT) Fact* getMC_MAN_R_MAX(void) { return _mapParameterName2Fact["MC_MAN_R_MAX"]; }
    Q_PROPERTY(Fact* MC_MAN_Y_MAX READ getMC_MAN_Y_MAX CONSTANT) Fact* getMC_MAN_Y_MAX(void) { return _mapParameterName2Fact["MC_MAN_Y_MAX"]; }
    Q_PROPERTY(Fact* MC_PITCHRATE_D READ getMC_PITCHRATE_D CONSTANT) Fact* getMC_PITCHRATE_D(void) { return _mapParameterName2Fact["MC_PITCHRATE_D"]; }
    Q_PROPERTY(Fact* MC_PITCHRATE_I READ getMC_PITCHRATE_I CONSTANT) Fact* getMC_PITCHRATE_I(void) { return _mapParameterName2Fact["MC_PITCHRATE_I"]; }
    Q_PROPERTY(Fact* MC_PITCHRATE_P READ getMC_PITCHRATE_P CONSTANT) Fact* getMC_PITCHRATE_P(void) { return _mapParameterName2Fact["MC_PITCHRATE_P"]; }
    Q_PROPERTY(Fact* MC_PITCH_P READ getMC_PITCH_P CONSTANT) Fact* getMC_PITCH_P(void) { return _mapParameterName2Fact["MC_PITCH_P"]; }
    Q_PROPERTY(Fact* MC_ROLLRATE_D READ getMC_ROLLRATE_D CONSTANT) Fact* getMC_ROLLRATE_D(void) { return _mapParameterName2Fact["MC_ROLLRATE_D"]; }
    Q_PROPERTY(Fact* MC_ROLLRATE_I READ getMC_ROLLRATE_I CONSTANT) Fact* getMC_ROLLRATE_I(void) { return _mapParameterName2Fact["MC_ROLLRATE_I"]; }
    Q_PROPERTY(Fact* MC_ROLLRATE_P READ getMC_ROLLRATE_P CONSTANT) Fact* getMC_ROLLRATE_P(void) { return _mapParameterName2Fact["MC_ROLLRATE_P"]; }
    Q_PROPERTY(Fact* MC_ROLL_P READ getMC_ROLL_P CONSTANT) Fact* getMC_ROLL_P(void) { return _mapParameterName2Fact["MC_ROLL_P"]; }
    Q_PROPERTY(Fact* MC_YAWRATE_D READ getMC_YAWRATE_D CONSTANT) Fact* getMC_YAWRATE_D(void) { return _mapParameterName2Fact["MC_YAWRATE_D"]; }
    Q_PROPERTY(Fact* MC_YAWRATE_I READ getMC_YAWRATE_I CONSTANT) Fact* getMC_YAWRATE_I(void) { return _mapParameterName2Fact["MC_YAWRATE_I"]; }
    Q_PROPERTY(Fact* MC_YAWRATE_MAX READ getMC_YAWRATE_MAX CONSTANT) Fact* getMC_YAWRATE_MAX(void) { return _mapParameterName2Fact["MC_YAWRATE_MAX"]; }
    Q_PROPERTY(Fact* MC_YAWRATE_P READ getMC_YAWRATE_P CONSTANT) Fact* getMC_YAWRATE_P(void) { return _mapParameterName2Fact["MC_YAWRATE_P"]; }
    Q_PROPERTY(Fact* MC_YAW_FF READ getMC_YAW_FF CONSTANT) Fact* getMC_YAW_FF(void) { return _mapParameterName2Fact["MC_YAW_FF"]; }
    Q_PROPERTY(Fact* MC_YAW_P READ getMC_YAW_P CONSTANT) Fact* getMC_YAW_P(void) { return _mapParameterName2Fact["MC_YAW_P"]; }
    Q_PROPERTY(Fact* MIS_ALTMODE READ getMIS_ALTMODE CONSTANT) Fact* getMIS_ALTMODE(void) { return _mapParameterName2Fact["MIS_ALTMODE"]; }
    Q_PROPERTY(Fact* MIS_DIST_1WP READ getMIS_DIST_1WP CONSTANT) Fact* getMIS_DIST_1WP(void) { return _mapParameterName2Fact["MIS_DIST_1WP"]; }
    Q_PROPERTY(Fact* MIS_ONBOARD_EN READ getMIS_ONBOARD_EN CONSTANT) Fact* getMIS_ONBOARD_EN(void) { return _mapParameterName2Fact["MIS_ONBOARD_EN"]; }
    Q_PROPERTY(Fact* MIS_TAKEOFF_ALT READ getMIS_TAKEOFF_ALT CONSTANT) Fact* getMIS_TAKEOFF_ALT(void) { return _mapParameterName2Fact["MIS_TAKEOFF_ALT"]; }
    Q_PROPERTY(Fact* MPC_LAND_SPEED READ getMPC_LAND_SPEED CONSTANT) Fact* getMPC_LAND_SPEED(void) { return _mapParameterName2Fact["MPC_LAND_SPEED"]; }
    Q_PROPERTY(Fact* MPC_THR_MAX READ getMPC_THR_MAX CONSTANT) Fact* getMPC_THR_MAX(void) { return _mapParameterName2Fact["MPC_THR_MAX"]; }
    Q_PROPERTY(Fact* MPC_THR_MIN READ getMPC_THR_MIN CONSTANT) Fact* getMPC_THR_MIN(void) { return _mapParameterName2Fact["MPC_THR_MIN"]; }
    Q_PROPERTY(Fact* MPC_TILTMAX_AIR READ getMPC_TILTMAX_AIR CONSTANT) Fact* getMPC_TILTMAX_AIR(void) { return _mapParameterName2Fact["MPC_TILTMAX_AIR"]; }
    Q_PROPERTY(Fact* MPC_TILTMAX_LND READ getMPC_TILTMAX_LND CONSTANT) Fact* getMPC_TILTMAX_LND(void) { return _mapParameterName2Fact["MPC_TILTMAX_LND"]; }
    Q_PROPERTY(Fact* MPC_XY_FF READ getMPC_XY_FF CONSTANT) Fact* getMPC_XY_FF(void) { return _mapParameterName2Fact["MPC_XY_FF"]; }
    Q_PROPERTY(Fact* MPC_XY_P READ getMPC_XY_P CONSTANT) Fact* getMPC_XY_P(void) { return _mapParameterName2Fact["MPC_XY_P"]; }
    Q_PROPERTY(Fact* MPC_XY_VEL_D READ getMPC_XY_VEL_D CONSTANT) Fact* getMPC_XY_VEL_D(void) { return _mapParameterName2Fact["MPC_XY_VEL_D"]; }
    Q_PROPERTY(Fact* MPC_XY_VEL_I READ getMPC_XY_VEL_I CONSTANT) Fact* getMPC_XY_VEL_I(void) { return _mapParameterName2Fact["MPC_XY_VEL_I"]; }
    Q_PROPERTY(Fact* MPC_XY_VEL_MAX READ getMPC_XY_VEL_MAX CONSTANT) Fact* getMPC_XY_VEL_MAX(void) { return _mapParameterName2Fact["MPC_XY_VEL_MAX"]; }
    Q_PROPERTY(Fact* MPC_XY_VEL_P READ getMPC_XY_VEL_P CONSTANT) Fact* getMPC_XY_VEL_P(void) { return _mapParameterName2Fact["MPC_XY_VEL_P"]; }
    Q_PROPERTY(Fact* MPC_Z_FF READ getMPC_Z_FF CONSTANT) Fact* getMPC_Z_FF(void) { return _mapParameterName2Fact["MPC_Z_FF"]; }
    Q_PROPERTY(Fact* MPC_Z_P READ getMPC_Z_P CONSTANT) Fact* getMPC_Z_P(void) { return _mapParameterName2Fact["MPC_Z_P"]; }
    Q_PROPERTY(Fact* MPC_Z_VEL_D READ getMPC_Z_VEL_D CONSTANT) Fact* getMPC_Z_VEL_D(void) { return _mapParameterName2Fact["MPC_Z_VEL_D"]; }
    Q_PROPERTY(Fact* MPC_Z_VEL_I READ getMPC_Z_VEL_I CONSTANT) Fact* getMPC_Z_VEL_I(void) { return _mapParameterName2Fact["MPC_Z_VEL_I"]; }
    Q_PROPERTY(Fact* MPC_Z_VEL_MAX READ getMPC_Z_VEL_MAX CONSTANT) Fact* getMPC_Z_VEL_MAX(void) { return _mapParameterName2Fact["MPC_Z_VEL_MAX"]; }
    Q_PROPERTY(Fact* MPC_Z_VEL_P READ getMPC_Z_VEL_P CONSTANT) Fact* getMPC_Z_VEL_P(void) { return _mapParameterName2Fact["MPC_Z_VEL_P"]; }
    Q_PROPERTY(Fact* MT_ACC_D READ getMT_ACC_D CONSTANT) Fact* getMT_ACC_D(void) { return _mapParameterName2Fact["MT_ACC_D"]; }
    Q_PROPERTY(Fact* MT_ACC_D_LP READ getMT_ACC_D_LP CONSTANT) Fact* getMT_ACC_D_LP(void) { return _mapParameterName2Fact["MT_ACC_D_LP"]; }
    Q_PROPERTY(Fact* MT_ACC_MAX READ getMT_ACC_MAX CONSTANT) Fact* getMT_ACC_MAX(void) { return _mapParameterName2Fact["MT_ACC_MAX"]; }
    Q_PROPERTY(Fact* MT_ACC_MIN READ getMT_ACC_MIN CONSTANT) Fact* getMT_ACC_MIN(void) { return _mapParameterName2Fact["MT_ACC_MIN"]; }
    Q_PROPERTY(Fact* MT_ACC_P READ getMT_ACC_P CONSTANT) Fact* getMT_ACC_P(void) { return _mapParameterName2Fact["MT_ACC_P"]; }
    Q_PROPERTY(Fact* MT_AD_LP READ getMT_AD_LP CONSTANT) Fact* getMT_AD_LP(void) { return _mapParameterName2Fact["MT_AD_LP"]; }
    Q_PROPERTY(Fact* MT_ALT_LP READ getMT_ALT_LP CONSTANT) Fact* getMT_ALT_LP(void) { return _mapParameterName2Fact["MT_ALT_LP"]; }
    Q_PROPERTY(Fact* MT_A_LP READ getMT_A_LP CONSTANT) Fact* getMT_A_LP(void) { return _mapParameterName2Fact["MT_A_LP"]; }
    Q_PROPERTY(Fact* MT_ENABLED READ getMT_ENABLED CONSTANT) Fact* getMT_ENABLED(void) { return _mapParameterName2Fact["MT_ENABLED"]; }
    Q_PROPERTY(Fact* MT_FPA_D READ getMT_FPA_D CONSTANT) Fact* getMT_FPA_D(void) { return _mapParameterName2Fact["MT_FPA_D"]; }
    Q_PROPERTY(Fact* MT_FPA_D_LP READ getMT_FPA_D_LP CONSTANT) Fact* getMT_FPA_D_LP(void) { return _mapParameterName2Fact["MT_FPA_D_LP"]; }
    Q_PROPERTY(Fact* MT_FPA_LP READ getMT_FPA_LP CONSTANT) Fact* getMT_FPA_LP(void) { return _mapParameterName2Fact["MT_FPA_LP"]; }
    Q_PROPERTY(Fact* MT_FPA_MAX READ getMT_FPA_MAX CONSTANT) Fact* getMT_FPA_MAX(void) { return _mapParameterName2Fact["MT_FPA_MAX"]; }
    Q_PROPERTY(Fact* MT_FPA_MIN READ getMT_FPA_MIN CONSTANT) Fact* getMT_FPA_MIN(void) { return _mapParameterName2Fact["MT_FPA_MIN"]; }
    Q_PROPERTY(Fact* MT_FPA_P READ getMT_FPA_P CONSTANT) Fact* getMT_FPA_P(void) { return _mapParameterName2Fact["MT_FPA_P"]; }
    Q_PROPERTY(Fact* MT_LND_PIT_MAX READ getMT_LND_PIT_MAX CONSTANT) Fact* getMT_LND_PIT_MAX(void) { return _mapParameterName2Fact["MT_LND_PIT_MAX"]; }
    Q_PROPERTY(Fact* MT_LND_PIT_MIN READ getMT_LND_PIT_MIN CONSTANT) Fact* getMT_LND_PIT_MIN(void) { return _mapParameterName2Fact["MT_LND_PIT_MIN"]; }
    Q_PROPERTY(Fact* MT_LND_THR_MAX READ getMT_LND_THR_MAX CONSTANT) Fact* getMT_LND_THR_MAX(void) { return _mapParameterName2Fact["MT_LND_THR_MAX"]; }
    Q_PROPERTY(Fact* MT_LND_THR_MIN READ getMT_LND_THR_MIN CONSTANT) Fact* getMT_LND_THR_MIN(void) { return _mapParameterName2Fact["MT_LND_THR_MIN"]; }
    Q_PROPERTY(Fact* MT_PIT_FF READ getMT_PIT_FF CONSTANT) Fact* getMT_PIT_FF(void) { return _mapParameterName2Fact["MT_PIT_FF"]; }
    Q_PROPERTY(Fact* MT_PIT_I READ getMT_PIT_I CONSTANT) Fact* getMT_PIT_I(void) { return _mapParameterName2Fact["MT_PIT_I"]; }
    Q_PROPERTY(Fact* MT_PIT_I_MAX READ getMT_PIT_I_MAX CONSTANT) Fact* getMT_PIT_I_MAX(void) { return _mapParameterName2Fact["MT_PIT_I_MAX"]; }
    Q_PROPERTY(Fact* MT_PIT_MAX READ getMT_PIT_MAX CONSTANT) Fact* getMT_PIT_MAX(void) { return _mapParameterName2Fact["MT_PIT_MAX"]; }
    Q_PROPERTY(Fact* MT_PIT_MIN READ getMT_PIT_MIN CONSTANT) Fact* getMT_PIT_MIN(void) { return _mapParameterName2Fact["MT_PIT_MIN"]; }
    Q_PROPERTY(Fact* MT_PIT_OFF READ getMT_PIT_OFF CONSTANT) Fact* getMT_PIT_OFF(void) { return _mapParameterName2Fact["MT_PIT_OFF"]; }
    Q_PROPERTY(Fact* MT_PIT_P READ getMT_PIT_P CONSTANT) Fact* getMT_PIT_P(void) { return _mapParameterName2Fact["MT_PIT_P"]; }
    Q_PROPERTY(Fact* MT_THR_FF READ getMT_THR_FF CONSTANT) Fact* getMT_THR_FF(void) { return _mapParameterName2Fact["MT_THR_FF"]; }
    Q_PROPERTY(Fact* MT_THR_I READ getMT_THR_I CONSTANT) Fact* getMT_THR_I(void) { return _mapParameterName2Fact["MT_THR_I"]; }
    Q_PROPERTY(Fact* MT_THR_I_MAX READ getMT_THR_I_MAX CONSTANT) Fact* getMT_THR_I_MAX(void) { return _mapParameterName2Fact["MT_THR_I_MAX"]; }
    Q_PROPERTY(Fact* MT_THR_MAX READ getMT_THR_MAX CONSTANT) Fact* getMT_THR_MAX(void) { return _mapParameterName2Fact["MT_THR_MAX"]; }
    Q_PROPERTY(Fact* MT_THR_MIN READ getMT_THR_MIN CONSTANT) Fact* getMT_THR_MIN(void) { return _mapParameterName2Fact["MT_THR_MIN"]; }
    Q_PROPERTY(Fact* MT_THR_OFF READ getMT_THR_OFF CONSTANT) Fact* getMT_THR_OFF(void) { return _mapParameterName2Fact["MT_THR_OFF"]; }
    Q_PROPERTY(Fact* MT_THR_P READ getMT_THR_P CONSTANT) Fact* getMT_THR_P(void) { return _mapParameterName2Fact["MT_THR_P"]; }
    Q_PROPERTY(Fact* MT_TKF_PIT_MAX READ getMT_TKF_PIT_MAX CONSTANT) Fact* getMT_TKF_PIT_MAX(void) { return _mapParameterName2Fact["MT_TKF_PIT_MAX"]; }
    Q_PROPERTY(Fact* MT_TKF_PIT_MIN READ getMT_TKF_PIT_MIN CONSTANT) Fact* getMT_TKF_PIT_MIN(void) { return _mapParameterName2Fact["MT_TKF_PIT_MIN"]; }
    Q_PROPERTY(Fact* MT_TKF_THR_MAX READ getMT_TKF_THR_MAX CONSTANT) Fact* getMT_TKF_THR_MAX(void) { return _mapParameterName2Fact["MT_TKF_THR_MAX"]; }
    Q_PROPERTY(Fact* MT_TKF_THR_MIN READ getMT_TKF_THR_MIN CONSTANT) Fact* getMT_TKF_THR_MIN(void) { return _mapParameterName2Fact["MT_TKF_THR_MIN"]; }
    Q_PROPERTY(Fact* MT_USP_PIT_MAX READ getMT_USP_PIT_MAX CONSTANT) Fact* getMT_USP_PIT_MAX(void) { return _mapParameterName2Fact["MT_USP_PIT_MAX"]; }
    Q_PROPERTY(Fact* MT_USP_PIT_MIN READ getMT_USP_PIT_MIN CONSTANT) Fact* getMT_USP_PIT_MIN(void) { return _mapParameterName2Fact["MT_USP_PIT_MIN"]; }
    Q_PROPERTY(Fact* MT_USP_THR_MAX READ getMT_USP_THR_MAX CONSTANT) Fact* getMT_USP_THR_MAX(void) { return _mapParameterName2Fact["MT_USP_THR_MAX"]; }
    Q_PROPERTY(Fact* MT_USP_THR_MIN READ getMT_USP_THR_MIN CONSTANT) Fact* getMT_USP_THR_MIN(void) { return _mapParameterName2Fact["MT_USP_THR_MIN"]; }
    Q_PROPERTY(Fact* NAV_ACC_RAD READ getNAV_ACC_RAD CONSTANT) Fact* getNAV_ACC_RAD(void) { return _mapParameterName2Fact["NAV_ACC_RAD"]; }
    Q_PROPERTY(Fact* NAV_AH_ALT READ getNAV_AH_ALT CONSTANT) Fact* getNAV_AH_ALT(void) { return _mapParameterName2Fact["NAV_AH_ALT"]; }
    Q_PROPERTY(Fact* NAV_AH_LAT READ getNAV_AH_LAT CONSTANT) Fact* getNAV_AH_LAT(void) { return _mapParameterName2Fact["NAV_AH_LAT"]; }
    Q_PROPERTY(Fact* NAV_AH_LON READ getNAV_AH_LON CONSTANT) Fact* getNAV_AH_LON(void) { return _mapParameterName2Fact["NAV_AH_LON"]; }
    Q_PROPERTY(Fact* NAV_DLL_AH_T READ getNAV_DLL_AH_T CONSTANT) Fact* getNAV_DLL_AH_T(void) { return _mapParameterName2Fact["NAV_DLL_AH_T"]; }
    Q_PROPERTY(Fact* NAV_DLL_CHSK READ getNAV_DLL_CHSK CONSTANT) Fact* getNAV_DLL_CHSK(void) { return _mapParameterName2Fact["NAV_DLL_CHSK"]; }
    Q_PROPERTY(Fact* NAV_DLL_CH_ALT READ getNAV_DLL_CH_ALT CONSTANT) Fact* getNAV_DLL_CH_ALT(void) { return _mapParameterName2Fact["NAV_DLL_CH_ALT"]; }
    Q_PROPERTY(Fact* NAV_DLL_CH_LAT READ getNAV_DLL_CH_LAT CONSTANT) Fact* getNAV_DLL_CH_LAT(void) { return _mapParameterName2Fact["NAV_DLL_CH_LAT"]; }
    Q_PROPERTY(Fact* NAV_DLL_CH_LON READ getNAV_DLL_CH_LON CONSTANT) Fact* getNAV_DLL_CH_LON(void) { return _mapParameterName2Fact["NAV_DLL_CH_LON"]; }
    Q_PROPERTY(Fact* NAV_DLL_CH_T READ getNAV_DLL_CH_T CONSTANT) Fact* getNAV_DLL_CH_T(void) { return _mapParameterName2Fact["NAV_DLL_CH_T"]; }
    Q_PROPERTY(Fact* NAV_DLL_N READ getNAV_DLL_N CONSTANT) Fact* getNAV_DLL_N(void) { return _mapParameterName2Fact["NAV_DLL_N"]; }
    Q_PROPERTY(Fact* NAV_DLL_OBC READ getNAV_DLL_OBC CONSTANT) Fact* getNAV_DLL_OBC(void) { return _mapParameterName2Fact["NAV_DLL_OBC"]; }
    Q_PROPERTY(Fact* NAV_GPSF_LT READ getNAV_GPSF_LT CONSTANT) Fact* getNAV_GPSF_LT(void) { return _mapParameterName2Fact["NAV_GPSF_LT"]; }
    Q_PROPERTY(Fact* NAV_GPSF_P READ getNAV_GPSF_P CONSTANT) Fact* getNAV_GPSF_P(void) { return _mapParameterName2Fact["NAV_GPSF_P"]; }
    Q_PROPERTY(Fact* NAV_GPSF_R READ getNAV_GPSF_R CONSTANT) Fact* getNAV_GPSF_R(void) { return _mapParameterName2Fact["NAV_GPSF_R"]; }
    Q_PROPERTY(Fact* NAV_GPSF_TR READ getNAV_GPSF_TR CONSTANT) Fact* getNAV_GPSF_TR(void) { return _mapParameterName2Fact["NAV_GPSF_TR"]; }
    Q_PROPERTY(Fact* NAV_LOITER_RAD READ getNAV_LOITER_RAD CONSTANT) Fact* getNAV_LOITER_RAD(void) { return _mapParameterName2Fact["NAV_LOITER_RAD"]; }
    Q_PROPERTY(Fact* NAV_RCL_LT READ getNAV_RCL_LT CONSTANT) Fact* getNAV_RCL_LT(void) { return _mapParameterName2Fact["NAV_RCL_LT"]; }
    Q_PROPERTY(Fact* NAV_RCL_OBC READ getNAV_RCL_OBC CONSTANT) Fact* getNAV_RCL_OBC(void) { return _mapParameterName2Fact["NAV_RCL_OBC"]; }
    Q_PROPERTY(Fact* PE_ABIAS_PNOISE READ getPE_ABIAS_PNOISE CONSTANT) Fact* getPE_ABIAS_PNOISE(void) { return _mapParameterName2Fact["PE_ABIAS_PNOISE"]; }
    Q_PROPERTY(Fact* PE_ACC_PNOISE READ getPE_ACC_PNOISE CONSTANT) Fact* getPE_ACC_PNOISE(void) { return _mapParameterName2Fact["PE_ACC_PNOISE"]; }
    Q_PROPERTY(Fact* PE_EAS_NOISE READ getPE_EAS_NOISE CONSTANT) Fact* getPE_EAS_NOISE(void) { return _mapParameterName2Fact["PE_EAS_NOISE"]; }
    Q_PROPERTY(Fact* PE_GBIAS_PNOISE READ getPE_GBIAS_PNOISE CONSTANT) Fact* getPE_GBIAS_PNOISE(void) { return _mapParameterName2Fact["PE_GBIAS_PNOISE"]; }
    Q_PROPERTY(Fact* PE_GPS_ALT_WGT READ getPE_GPS_ALT_WGT CONSTANT) Fact* getPE_GPS_ALT_WGT(void) { return _mapParameterName2Fact["PE_GPS_ALT_WGT"]; }
    Q_PROPERTY(Fact* PE_GYRO_PNOISE READ getPE_GYRO_PNOISE CONSTANT) Fact* getPE_GYRO_PNOISE(void) { return _mapParameterName2Fact["PE_GYRO_PNOISE"]; }
    Q_PROPERTY(Fact* PE_HGT_DELAY_MS READ getPE_HGT_DELAY_MS CONSTANT) Fact* getPE_HGT_DELAY_MS(void) { return _mapParameterName2Fact["PE_HGT_DELAY_MS"]; }
    Q_PROPERTY(Fact* PE_MAGB_PNOISE READ getPE_MAGB_PNOISE CONSTANT) Fact* getPE_MAGB_PNOISE(void) { return _mapParameterName2Fact["PE_MAGB_PNOISE"]; }
    Q_PROPERTY(Fact* PE_MAGE_PNOISE READ getPE_MAGE_PNOISE CONSTANT) Fact* getPE_MAGE_PNOISE(void) { return _mapParameterName2Fact["PE_MAGE_PNOISE"]; }
    Q_PROPERTY(Fact* PE_MAG_DELAY_MS READ getPE_MAG_DELAY_MS CONSTANT) Fact* getPE_MAG_DELAY_MS(void) { return _mapParameterName2Fact["PE_MAG_DELAY_MS"]; }
    Q_PROPERTY(Fact* PE_MAG_NOISE READ getPE_MAG_NOISE CONSTANT) Fact* getPE_MAG_NOISE(void) { return _mapParameterName2Fact["PE_MAG_NOISE"]; }
    Q_PROPERTY(Fact* PE_POSDEV_INIT READ getPE_POSDEV_INIT CONSTANT) Fact* getPE_POSDEV_INIT(void) { return _mapParameterName2Fact["PE_POSDEV_INIT"]; }
    Q_PROPERTY(Fact* PE_POSD_NOISE READ getPE_POSD_NOISE CONSTANT) Fact* getPE_POSD_NOISE(void) { return _mapParameterName2Fact["PE_POSD_NOISE"]; }
    Q_PROPERTY(Fact* PE_POSNE_NOISE READ getPE_POSNE_NOISE CONSTANT) Fact* getPE_POSNE_NOISE(void) { return _mapParameterName2Fact["PE_POSNE_NOISE"]; }
    Q_PROPERTY(Fact* PE_POS_DELAY_MS READ getPE_POS_DELAY_MS CONSTANT) Fact* getPE_POS_DELAY_MS(void) { return _mapParameterName2Fact["PE_POS_DELAY_MS"]; }
    Q_PROPERTY(Fact* PE_TAS_DELAY_MS READ getPE_TAS_DELAY_MS CONSTANT) Fact* getPE_TAS_DELAY_MS(void) { return _mapParameterName2Fact["PE_TAS_DELAY_MS"]; }
    Q_PROPERTY(Fact* PE_VELD_NOISE READ getPE_VELD_NOISE CONSTANT) Fact* getPE_VELD_NOISE(void) { return _mapParameterName2Fact["PE_VELD_NOISE"]; }
    Q_PROPERTY(Fact* PE_VELNE_NOISE READ getPE_VELNE_NOISE CONSTANT) Fact* getPE_VELNE_NOISE(void) { return _mapParameterName2Fact["PE_VELNE_NOISE"]; }
    Q_PROPERTY(Fact* PE_VEL_DELAY_MS READ getPE_VEL_DELAY_MS CONSTANT) Fact* getPE_VEL_DELAY_MS(void) { return _mapParameterName2Fact["PE_VEL_DELAY_MS"]; }
    Q_PROPERTY(Fact* RC10_DZ READ getRC10_DZ CONSTANT) Fact* getRC10_DZ(void) { return _mapParameterName2Fact["RC10_DZ"]; }
    Q_PROPERTY(Fact* RC10_MAX READ getRC10_MAX CONSTANT) Fact* getRC10_MAX(void) { return _mapParameterName2Fact["RC10_MAX"]; }
    Q_PROPERTY(Fact* RC10_MIN READ getRC10_MIN CONSTANT) Fact* getRC10_MIN(void) { return _mapParameterName2Fact["RC10_MIN"]; }
    Q_PROPERTY(Fact* RC10_REV READ getRC10_REV CONSTANT) Fact* getRC10_REV(void) { return _mapParameterName2Fact["RC10_REV"]; }
    Q_PROPERTY(Fact* RC10_TRIM READ getRC10_TRIM CONSTANT) Fact* getRC10_TRIM(void) { return _mapParameterName2Fact["RC10_TRIM"]; }
    Q_PROPERTY(Fact* RC11_DZ READ getRC11_DZ CONSTANT) Fact* getRC11_DZ(void) { return _mapParameterName2Fact["RC11_DZ"]; }
    Q_PROPERTY(Fact* RC11_MAX READ getRC11_MAX CONSTANT) Fact* getRC11_MAX(void) { return _mapParameterName2Fact["RC11_MAX"]; }
    Q_PROPERTY(Fact* RC11_MIN READ getRC11_MIN CONSTANT) Fact* getRC11_MIN(void) { return _mapParameterName2Fact["RC11_MIN"]; }
    Q_PROPERTY(Fact* RC11_REV READ getRC11_REV CONSTANT) Fact* getRC11_REV(void) { return _mapParameterName2Fact["RC11_REV"]; }
    Q_PROPERTY(Fact* RC11_TRIM READ getRC11_TRIM CONSTANT) Fact* getRC11_TRIM(void) { return _mapParameterName2Fact["RC11_TRIM"]; }
    Q_PROPERTY(Fact* RC12_DZ READ getRC12_DZ CONSTANT) Fact* getRC12_DZ(void) { return _mapParameterName2Fact["RC12_DZ"]; }
    Q_PROPERTY(Fact* RC12_MAX READ getRC12_MAX CONSTANT) Fact* getRC12_MAX(void) { return _mapParameterName2Fact["RC12_MAX"]; }
    Q_PROPERTY(Fact* RC12_MIN READ getRC12_MIN CONSTANT) Fact* getRC12_MIN(void) { return _mapParameterName2Fact["RC12_MIN"]; }
    Q_PROPERTY(Fact* RC12_REV READ getRC12_REV CONSTANT) Fact* getRC12_REV(void) { return _mapParameterName2Fact["RC12_REV"]; }
    Q_PROPERTY(Fact* RC12_TRIM READ getRC12_TRIM CONSTANT) Fact* getRC12_TRIM(void) { return _mapParameterName2Fact["RC12_TRIM"]; }
    Q_PROPERTY(Fact* RC13_DZ READ getRC13_DZ CONSTANT) Fact* getRC13_DZ(void) { return _mapParameterName2Fact["RC13_DZ"]; }
    Q_PROPERTY(Fact* RC13_MAX READ getRC13_MAX CONSTANT) Fact* getRC13_MAX(void) { return _mapParameterName2Fact["RC13_MAX"]; }
    Q_PROPERTY(Fact* RC13_MIN READ getRC13_MIN CONSTANT) Fact* getRC13_MIN(void) { return _mapParameterName2Fact["RC13_MIN"]; }
    Q_PROPERTY(Fact* RC13_REV READ getRC13_REV CONSTANT) Fact* getRC13_REV(void) { return _mapParameterName2Fact["RC13_REV"]; }
    Q_PROPERTY(Fact* RC13_TRIM READ getRC13_TRIM CONSTANT) Fact* getRC13_TRIM(void) { return _mapParameterName2Fact["RC13_TRIM"]; }
    Q_PROPERTY(Fact* RC14_DZ READ getRC14_DZ CONSTANT) Fact* getRC14_DZ(void) { return _mapParameterName2Fact["RC14_DZ"]; }
    Q_PROPERTY(Fact* RC14_MAX READ getRC14_MAX CONSTANT) Fact* getRC14_MAX(void) { return _mapParameterName2Fact["RC14_MAX"]; }
    Q_PROPERTY(Fact* RC14_MIN READ getRC14_MIN CONSTANT) Fact* getRC14_MIN(void) { return _mapParameterName2Fact["RC14_MIN"]; }
    Q_PROPERTY(Fact* RC14_REV READ getRC14_REV CONSTANT) Fact* getRC14_REV(void) { return _mapParameterName2Fact["RC14_REV"]; }
    Q_PROPERTY(Fact* RC14_TRIM READ getRC14_TRIM CONSTANT) Fact* getRC14_TRIM(void) { return _mapParameterName2Fact["RC14_TRIM"]; }
    Q_PROPERTY(Fact* RC15_DZ READ getRC15_DZ CONSTANT) Fact* getRC15_DZ(void) { return _mapParameterName2Fact["RC15_DZ"]; }
    Q_PROPERTY(Fact* RC15_MAX READ getRC15_MAX CONSTANT) Fact* getRC15_MAX(void) { return _mapParameterName2Fact["RC15_MAX"]; }
    Q_PROPERTY(Fact* RC15_MIN READ getRC15_MIN CONSTANT) Fact* getRC15_MIN(void) { return _mapParameterName2Fact["RC15_MIN"]; }
    Q_PROPERTY(Fact* RC15_REV READ getRC15_REV CONSTANT) Fact* getRC15_REV(void) { return _mapParameterName2Fact["RC15_REV"]; }
    Q_PROPERTY(Fact* RC15_TRIM READ getRC15_TRIM CONSTANT) Fact* getRC15_TRIM(void) { return _mapParameterName2Fact["RC15_TRIM"]; }
    Q_PROPERTY(Fact* RC16_DZ READ getRC16_DZ CONSTANT) Fact* getRC16_DZ(void) { return _mapParameterName2Fact["RC16_DZ"]; }
    Q_PROPERTY(Fact* RC16_MAX READ getRC16_MAX CONSTANT) Fact* getRC16_MAX(void) { return _mapParameterName2Fact["RC16_MAX"]; }
    Q_PROPERTY(Fact* RC16_MIN READ getRC16_MIN CONSTANT) Fact* getRC16_MIN(void) { return _mapParameterName2Fact["RC16_MIN"]; }
    Q_PROPERTY(Fact* RC16_REV READ getRC16_REV CONSTANT) Fact* getRC16_REV(void) { return _mapParameterName2Fact["RC16_REV"]; }
    Q_PROPERTY(Fact* RC16_TRIM READ getRC16_TRIM CONSTANT) Fact* getRC16_TRIM(void) { return _mapParameterName2Fact["RC16_TRIM"]; }
    Q_PROPERTY(Fact* RC17_DZ READ getRC17_DZ CONSTANT) Fact* getRC17_DZ(void) { return _mapParameterName2Fact["RC17_DZ"]; }
    Q_PROPERTY(Fact* RC17_MAX READ getRC17_MAX CONSTANT) Fact* getRC17_MAX(void) { return _mapParameterName2Fact["RC17_MAX"]; }
    Q_PROPERTY(Fact* RC17_MIN READ getRC17_MIN CONSTANT) Fact* getRC17_MIN(void) { return _mapParameterName2Fact["RC17_MIN"]; }
    Q_PROPERTY(Fact* RC17_REV READ getRC17_REV CONSTANT) Fact* getRC17_REV(void) { return _mapParameterName2Fact["RC17_REV"]; }
    Q_PROPERTY(Fact* RC17_TRIM READ getRC17_TRIM CONSTANT) Fact* getRC17_TRIM(void) { return _mapParameterName2Fact["RC17_TRIM"]; }
    Q_PROPERTY(Fact* RC18_DZ READ getRC18_DZ CONSTANT) Fact* getRC18_DZ(void) { return _mapParameterName2Fact["RC18_DZ"]; }
    Q_PROPERTY(Fact* RC18_MAX READ getRC18_MAX CONSTANT) Fact* getRC18_MAX(void) { return _mapParameterName2Fact["RC18_MAX"]; }
    Q_PROPERTY(Fact* RC18_MIN READ getRC18_MIN CONSTANT) Fact* getRC18_MIN(void) { return _mapParameterName2Fact["RC18_MIN"]; }
    Q_PROPERTY(Fact* RC18_REV READ getRC18_REV CONSTANT) Fact* getRC18_REV(void) { return _mapParameterName2Fact["RC18_REV"]; }
    Q_PROPERTY(Fact* RC18_TRIM READ getRC18_TRIM CONSTANT) Fact* getRC18_TRIM(void) { return _mapParameterName2Fact["RC18_TRIM"]; }
    Q_PROPERTY(Fact* RC1_DZ READ getRC1_DZ CONSTANT) Fact* getRC1_DZ(void) { return _mapParameterName2Fact["RC1_DZ"]; }
    Q_PROPERTY(Fact* RC1_MAX READ getRC1_MAX CONSTANT) Fact* getRC1_MAX(void) { return _mapParameterName2Fact["RC1_MAX"]; }
    Q_PROPERTY(Fact* RC1_MIN READ getRC1_MIN CONSTANT) Fact* getRC1_MIN(void) { return _mapParameterName2Fact["RC1_MIN"]; }
    Q_PROPERTY(Fact* RC1_REV READ getRC1_REV CONSTANT) Fact* getRC1_REV(void) { return _mapParameterName2Fact["RC1_REV"]; }
    Q_PROPERTY(Fact* RC1_TRIM READ getRC1_TRIM CONSTANT) Fact* getRC1_TRIM(void) { return _mapParameterName2Fact["RC1_TRIM"]; }
    Q_PROPERTY(Fact* RC2_DZ READ getRC2_DZ CONSTANT) Fact* getRC2_DZ(void) { return _mapParameterName2Fact["RC2_DZ"]; }
    Q_PROPERTY(Fact* RC2_MAX READ getRC2_MAX CONSTANT) Fact* getRC2_MAX(void) { return _mapParameterName2Fact["RC2_MAX"]; }
    Q_PROPERTY(Fact* RC2_MIN READ getRC2_MIN CONSTANT) Fact* getRC2_MIN(void) { return _mapParameterName2Fact["RC2_MIN"]; }
    Q_PROPERTY(Fact* RC2_REV READ getRC2_REV CONSTANT) Fact* getRC2_REV(void) { return _mapParameterName2Fact["RC2_REV"]; }
    Q_PROPERTY(Fact* RC2_TRIM READ getRC2_TRIM CONSTANT) Fact* getRC2_TRIM(void) { return _mapParameterName2Fact["RC2_TRIM"]; }
    Q_PROPERTY(Fact* RC3_DZ READ getRC3_DZ CONSTANT) Fact* getRC3_DZ(void) { return _mapParameterName2Fact["RC3_DZ"]; }
    Q_PROPERTY(Fact* RC3_MAX READ getRC3_MAX CONSTANT) Fact* getRC3_MAX(void) { return _mapParameterName2Fact["RC3_MAX"]; }
    Q_PROPERTY(Fact* RC3_MIN READ getRC3_MIN CONSTANT) Fact* getRC3_MIN(void) { return _mapParameterName2Fact["RC3_MIN"]; }
    Q_PROPERTY(Fact* RC3_REV READ getRC3_REV CONSTANT) Fact* getRC3_REV(void) { return _mapParameterName2Fact["RC3_REV"]; }
    Q_PROPERTY(Fact* RC3_TRIM READ getRC3_TRIM CONSTANT) Fact* getRC3_TRIM(void) { return _mapParameterName2Fact["RC3_TRIM"]; }
    Q_PROPERTY(Fact* RC4_DZ READ getRC4_DZ CONSTANT) Fact* getRC4_DZ(void) { return _mapParameterName2Fact["RC4_DZ"]; }
    Q_PROPERTY(Fact* RC4_MAX READ getRC4_MAX CONSTANT) Fact* getRC4_MAX(void) { return _mapParameterName2Fact["RC4_MAX"]; }
    Q_PROPERTY(Fact* RC4_MIN READ getRC4_MIN CONSTANT) Fact* getRC4_MIN(void) { return _mapParameterName2Fact["RC4_MIN"]; }
    Q_PROPERTY(Fact* RC4_REV READ getRC4_REV CONSTANT) Fact* getRC4_REV(void) { return _mapParameterName2Fact["RC4_REV"]; }
    Q_PROPERTY(Fact* RC4_TRIM READ getRC4_TRIM CONSTANT) Fact* getRC4_TRIM(void) { return _mapParameterName2Fact["RC4_TRIM"]; }
    Q_PROPERTY(Fact* RC5_DZ READ getRC5_DZ CONSTANT) Fact* getRC5_DZ(void) { return _mapParameterName2Fact["RC5_DZ"]; }
    Q_PROPERTY(Fact* RC5_MAX READ getRC5_MAX CONSTANT) Fact* getRC5_MAX(void) { return _mapParameterName2Fact["RC5_MAX"]; }
    Q_PROPERTY(Fact* RC5_MIN READ getRC5_MIN CONSTANT) Fact* getRC5_MIN(void) { return _mapParameterName2Fact["RC5_MIN"]; }
    Q_PROPERTY(Fact* RC5_REV READ getRC5_REV CONSTANT) Fact* getRC5_REV(void) { return _mapParameterName2Fact["RC5_REV"]; }
    Q_PROPERTY(Fact* RC5_TRIM READ getRC5_TRIM CONSTANT) Fact* getRC5_TRIM(void) { return _mapParameterName2Fact["RC5_TRIM"]; }
    Q_PROPERTY(Fact* RC6_DZ READ getRC6_DZ CONSTANT) Fact* getRC6_DZ(void) { return _mapParameterName2Fact["RC6_DZ"]; }
    Q_PROPERTY(Fact* RC6_MAX READ getRC6_MAX CONSTANT) Fact* getRC6_MAX(void) { return _mapParameterName2Fact["RC6_MAX"]; }
    Q_PROPERTY(Fact* RC6_MIN READ getRC6_MIN CONSTANT) Fact* getRC6_MIN(void) { return _mapParameterName2Fact["RC6_MIN"]; }
    Q_PROPERTY(Fact* RC6_REV READ getRC6_REV CONSTANT) Fact* getRC6_REV(void) { return _mapParameterName2Fact["RC6_REV"]; }
    Q_PROPERTY(Fact* RC6_TRIM READ getRC6_TRIM CONSTANT) Fact* getRC6_TRIM(void) { return _mapParameterName2Fact["RC6_TRIM"]; }
    Q_PROPERTY(Fact* RC7_DZ READ getRC7_DZ CONSTANT) Fact* getRC7_DZ(void) { return _mapParameterName2Fact["RC7_DZ"]; }
    Q_PROPERTY(Fact* RC7_MAX READ getRC7_MAX CONSTANT) Fact* getRC7_MAX(void) { return _mapParameterName2Fact["RC7_MAX"]; }
    Q_PROPERTY(Fact* RC7_MIN READ getRC7_MIN CONSTANT) Fact* getRC7_MIN(void) { return _mapParameterName2Fact["RC7_MIN"]; }
    Q_PROPERTY(Fact* RC7_REV READ getRC7_REV CONSTANT) Fact* getRC7_REV(void) { return _mapParameterName2Fact["RC7_REV"]; }
    Q_PROPERTY(Fact* RC7_TRIM READ getRC7_TRIM CONSTANT) Fact* getRC7_TRIM(void) { return _mapParameterName2Fact["RC7_TRIM"]; }
    Q_PROPERTY(Fact* RC8_DZ READ getRC8_DZ CONSTANT) Fact* getRC8_DZ(void) { return _mapParameterName2Fact["RC8_DZ"]; }
    Q_PROPERTY(Fact* RC8_MAX READ getRC8_MAX CONSTANT) Fact* getRC8_MAX(void) { return _mapParameterName2Fact["RC8_MAX"]; }
    Q_PROPERTY(Fact* RC8_MIN READ getRC8_MIN CONSTANT) Fact* getRC8_MIN(void) { return _mapParameterName2Fact["RC8_MIN"]; }
    Q_PROPERTY(Fact* RC8_REV READ getRC8_REV CONSTANT) Fact* getRC8_REV(void) { return _mapParameterName2Fact["RC8_REV"]; }
    Q_PROPERTY(Fact* RC8_TRIM READ getRC8_TRIM CONSTANT) Fact* getRC8_TRIM(void) { return _mapParameterName2Fact["RC8_TRIM"]; }
    Q_PROPERTY(Fact* RC9_DZ READ getRC9_DZ CONSTANT) Fact* getRC9_DZ(void) { return _mapParameterName2Fact["RC9_DZ"]; }
    Q_PROPERTY(Fact* RC9_MAX READ getRC9_MAX CONSTANT) Fact* getRC9_MAX(void) { return _mapParameterName2Fact["RC9_MAX"]; }
    Q_PROPERTY(Fact* RC9_MIN READ getRC9_MIN CONSTANT) Fact* getRC9_MIN(void) { return _mapParameterName2Fact["RC9_MIN"]; }
    Q_PROPERTY(Fact* RC9_REV READ getRC9_REV CONSTANT) Fact* getRC9_REV(void) { return _mapParameterName2Fact["RC9_REV"]; }
    Q_PROPERTY(Fact* RC9_TRIM READ getRC9_TRIM CONSTANT) Fact* getRC9_TRIM(void) { return _mapParameterName2Fact["RC9_TRIM"]; }
    Q_PROPERTY(Fact* RC_ACRO_TH READ getRC_ACRO_TH CONSTANT) Fact* getRC_ACRO_TH(void) { return _mapParameterName2Fact["RC_ACRO_TH"]; }
    Q_PROPERTY(Fact* RC_ASSIST_TH READ getRC_ASSIST_TH CONSTANT) Fact* getRC_ASSIST_TH(void) { return _mapParameterName2Fact["RC_ASSIST_TH"]; }
    Q_PROPERTY(Fact* RC_AUTO_TH READ getRC_AUTO_TH CONSTANT) Fact* getRC_AUTO_TH(void) { return _mapParameterName2Fact["RC_AUTO_TH"]; }
    Q_PROPERTY(Fact* RC_DSM_BIND READ getRC_DSM_BIND CONSTANT) Fact* getRC_DSM_BIND(void) { return _mapParameterName2Fact["RC_DSM_BIND"]; }
    Q_PROPERTY(Fact* RC_FAILS_THR READ getRC_FAILS_THR CONSTANT) Fact* getRC_FAILS_THR(void) { return _mapParameterName2Fact["RC_FAILS_THR"]; }
    Q_PROPERTY(Fact* RC_LOITER_TH READ getRC_LOITER_TH CONSTANT) Fact* getRC_LOITER_TH(void) { return _mapParameterName2Fact["RC_LOITER_TH"]; }
    Q_PROPERTY(Fact* RC_MAP_ACRO_SW READ getRC_MAP_ACRO_SW CONSTANT) Fact* getRC_MAP_ACRO_SW(void) { return _mapParameterName2Fact["RC_MAP_ACRO_SW"]; }
    Q_PROPERTY(Fact* RC_MAP_AUX1 READ getRC_MAP_AUX1 CONSTANT) Fact* getRC_MAP_AUX1(void) { return _mapParameterName2Fact["RC_MAP_AUX1"]; }
    Q_PROPERTY(Fact* RC_MAP_AUX2 READ getRC_MAP_AUX2 CONSTANT) Fact* getRC_MAP_AUX2(void) { return _mapParameterName2Fact["RC_MAP_AUX2"]; }
    Q_PROPERTY(Fact* RC_MAP_AUX3 READ getRC_MAP_AUX3 CONSTANT) Fact* getRC_MAP_AUX3(void) { return _mapParameterName2Fact["RC_MAP_AUX3"]; }
    Q_PROPERTY(Fact* RC_MAP_FAILSAFE READ getRC_MAP_FAILSAFE CONSTANT) Fact* getRC_MAP_FAILSAFE(void) { return _mapParameterName2Fact["RC_MAP_FAILSAFE"]; }
    Q_PROPERTY(Fact* RC_MAP_FLAPS READ getRC_MAP_FLAPS CONSTANT) Fact* getRC_MAP_FLAPS(void) { return _mapParameterName2Fact["RC_MAP_FLAPS"]; }
    Q_PROPERTY(Fact* RC_MAP_LOITER_SW READ getRC_MAP_LOITER_SW CONSTANT) Fact* getRC_MAP_LOITER_SW(void) { return _mapParameterName2Fact["RC_MAP_LOITER_SW"]; }
    Q_PROPERTY(Fact* RC_MAP_MODE_SW READ getRC_MAP_MODE_SW CONSTANT) Fact* getRC_MAP_MODE_SW(void) { return _mapParameterName2Fact["RC_MAP_MODE_SW"]; }
    Q_PROPERTY(Fact* RC_MAP_OFFB_SW READ getRC_MAP_OFFB_SW CONSTANT) Fact* getRC_MAP_OFFB_SW(void) { return _mapParameterName2Fact["RC_MAP_OFFB_SW"]; }
    Q_PROPERTY(Fact* RC_MAP_PITCH READ getRC_MAP_PITCH CONSTANT) Fact* getRC_MAP_PITCH(void) { return _mapParameterName2Fact["RC_MAP_PITCH"]; }
    Q_PROPERTY(Fact* RC_MAP_POSCTL_SW READ getRC_MAP_POSCTL_SW CONSTANT) Fact* getRC_MAP_POSCTL_SW(void) { return _mapParameterName2Fact["RC_MAP_POSCTL_SW"]; }
    Q_PROPERTY(Fact* RC_MAP_RETURN_SW READ getRC_MAP_RETURN_SW CONSTANT) Fact* getRC_MAP_RETURN_SW(void) { return _mapParameterName2Fact["RC_MAP_RETURN_SW"]; }
    Q_PROPERTY(Fact* RC_MAP_ROLL READ getRC_MAP_ROLL CONSTANT) Fact* getRC_MAP_ROLL(void) { return _mapParameterName2Fact["RC_MAP_ROLL"]; }
    Q_PROPERTY(Fact* RC_MAP_THROTTLE READ getRC_MAP_THROTTLE CONSTANT) Fact* getRC_MAP_THROTTLE(void) { return _mapParameterName2Fact["RC_MAP_THROTTLE"]; }
    Q_PROPERTY(Fact* RC_MAP_YAW READ getRC_MAP_YAW CONSTANT) Fact* getRC_MAP_YAW(void) { return _mapParameterName2Fact["RC_MAP_YAW"]; }
    Q_PROPERTY(Fact* RC_OFFB_TH READ getRC_OFFB_TH CONSTANT) Fact* getRC_OFFB_TH(void) { return _mapParameterName2Fact["RC_OFFB_TH"]; }
    Q_PROPERTY(Fact* RC_POSCTL_TH READ getRC_POSCTL_TH CONSTANT) Fact* getRC_POSCTL_TH(void) { return _mapParameterName2Fact["RC_POSCTL_TH"]; }
    Q_PROPERTY(Fact* RC_RETURN_TH READ getRC_RETURN_TH CONSTANT) Fact* getRC_RETURN_TH(void) { return _mapParameterName2Fact["RC_RETURN_TH"]; }
    Q_PROPERTY(Fact* RTL_DESCEND_ALT READ getRTL_DESCEND_ALT CONSTANT) Fact* getRTL_DESCEND_ALT(void) { return _mapParameterName2Fact["RTL_DESCEND_ALT"]; }
    Q_PROPERTY(Fact* RTL_LAND_DELAY READ getRTL_LAND_DELAY CONSTANT) Fact* getRTL_LAND_DELAY(void) { return _mapParameterName2Fact["RTL_LAND_DELAY"]; }
    Q_PROPERTY(Fact* RTL_LOITER_RAD READ getRTL_LOITER_RAD CONSTANT) Fact* getRTL_LOITER_RAD(void) { return _mapParameterName2Fact["RTL_LOITER_RAD"]; }
    Q_PROPERTY(Fact* RTL_RETURN_ALT READ getRTL_RETURN_ALT CONSTANT) Fact* getRTL_RETURN_ALT(void) { return _mapParameterName2Fact["RTL_RETURN_ALT"]; }
    Q_PROPERTY(Fact* SDLOG_EXT READ getSDLOG_EXT CONSTANT) Fact* getSDLOG_EXT(void) { return _mapParameterName2Fact["SDLOG_EXT"]; }
    Q_PROPERTY(Fact* SDLOG_RATE READ getSDLOG_RATE CONSTANT) Fact* getSDLOG_RATE(void) { return _mapParameterName2Fact["SDLOG_RATE"]; }
    Q_PROPERTY(Fact* SENS_ACC_XOFF READ getSENS_ACC_XOFF CONSTANT) Fact* getSENS_ACC_XOFF(void) { return _mapParameterName2Fact["SENS_ACC_XOFF"]; }
    Q_PROPERTY(Fact* SENS_ACC_XSCALE READ getSENS_ACC_XSCALE CONSTANT) Fact* getSENS_ACC_XSCALE(void) { return _mapParameterName2Fact["SENS_ACC_XSCALE"]; }
    Q_PROPERTY(Fact* SENS_ACC_YOFF READ getSENS_ACC_YOFF CONSTANT) Fact* getSENS_ACC_YOFF(void) { return _mapParameterName2Fact["SENS_ACC_YOFF"]; }
    Q_PROPERTY(Fact* SENS_ACC_YSCALE READ getSENS_ACC_YSCALE CONSTANT) Fact* getSENS_ACC_YSCALE(void) { return _mapParameterName2Fact["SENS_ACC_YSCALE"]; }
    Q_PROPERTY(Fact* SENS_ACC_ZOFF READ getSENS_ACC_ZOFF CONSTANT) Fact* getSENS_ACC_ZOFF(void) { return _mapParameterName2Fact["SENS_ACC_ZOFF"]; }
    Q_PROPERTY(Fact* SENS_ACC_ZSCALE READ getSENS_ACC_ZSCALE CONSTANT) Fact* getSENS_ACC_ZSCALE(void) { return _mapParameterName2Fact["SENS_ACC_ZSCALE"]; }
    Q_PROPERTY(Fact* SENS_BARO_QNH READ getSENS_BARO_QNH CONSTANT) Fact* getSENS_BARO_QNH(void) { return _mapParameterName2Fact["SENS_BARO_QNH"]; }
    Q_PROPERTY(Fact* SENS_BOARD_ROT READ getSENS_BOARD_ROT CONSTANT) Fact* getSENS_BOARD_ROT(void) { return _mapParameterName2Fact["SENS_BOARD_ROT"]; }
    Q_PROPERTY(Fact* SENS_BOARD_X_OFF READ getSENS_BOARD_X_OFF CONSTANT) Fact* getSENS_BOARD_X_OFF(void) { return _mapParameterName2Fact["SENS_BOARD_X_OFF"]; }
    Q_PROPERTY(Fact* SENS_BOARD_Y_OFF READ getSENS_BOARD_Y_OFF CONSTANT) Fact* getSENS_BOARD_Y_OFF(void) { return _mapParameterName2Fact["SENS_BOARD_Y_OFF"]; }
    Q_PROPERTY(Fact* SENS_BOARD_Z_OFF READ getSENS_BOARD_Z_OFF CONSTANT) Fact* getSENS_BOARD_Z_OFF(void) { return _mapParameterName2Fact["SENS_BOARD_Z_OFF"]; }
    Q_PROPERTY(Fact* SENS_DPRES_ANSC READ getSENS_DPRES_ANSC CONSTANT) Fact* getSENS_DPRES_ANSC(void) { return _mapParameterName2Fact["SENS_DPRES_ANSC"]; }
    Q_PROPERTY(Fact* SENS_DPRES_OFF READ getSENS_DPRES_OFF CONSTANT) Fact* getSENS_DPRES_OFF(void) { return _mapParameterName2Fact["SENS_DPRES_OFF"]; }
    Q_PROPERTY(Fact* SENS_EXT_MAG READ getSENS_EXT_MAG CONSTANT) Fact* getSENS_EXT_MAG(void) { return _mapParameterName2Fact["SENS_EXT_MAG"]; }
    Q_PROPERTY(Fact* SENS_EXT_MAG_ROT READ getSENS_EXT_MAG_ROT CONSTANT) Fact* getSENS_EXT_MAG_ROT(void) { return _mapParameterName2Fact["SENS_EXT_MAG_ROT"]; }
    Q_PROPERTY(Fact* SENS_GYRO_XOFF READ getSENS_GYRO_XOFF CONSTANT) Fact* getSENS_GYRO_XOFF(void) { return _mapParameterName2Fact["SENS_GYRO_XOFF"]; }
    Q_PROPERTY(Fact* SENS_GYRO_XSCALE READ getSENS_GYRO_XSCALE CONSTANT) Fact* getSENS_GYRO_XSCALE(void) { return _mapParameterName2Fact["SENS_GYRO_XSCALE"]; }
    Q_PROPERTY(Fact* SENS_GYRO_YOFF READ getSENS_GYRO_YOFF CONSTANT) Fact* getSENS_GYRO_YOFF(void) { return _mapParameterName2Fact["SENS_GYRO_YOFF"]; }
    Q_PROPERTY(Fact* SENS_GYRO_YSCALE READ getSENS_GYRO_YSCALE CONSTANT) Fact* getSENS_GYRO_YSCALE(void) { return _mapParameterName2Fact["SENS_GYRO_YSCALE"]; }
    Q_PROPERTY(Fact* SENS_GYRO_ZOFF READ getSENS_GYRO_ZOFF CONSTANT) Fact* getSENS_GYRO_ZOFF(void) { return _mapParameterName2Fact["SENS_GYRO_ZOFF"]; }
    Q_PROPERTY(Fact* SENS_GYRO_ZSCALE READ getSENS_GYRO_ZSCALE CONSTANT) Fact* getSENS_GYRO_ZSCALE(void) { return _mapParameterName2Fact["SENS_GYRO_ZSCALE"]; }
    Q_PROPERTY(Fact* SENS_MAG_XOFF READ getSENS_MAG_XOFF CONSTANT) Fact* getSENS_MAG_XOFF(void) { return _mapParameterName2Fact["SENS_MAG_XOFF"]; }
    Q_PROPERTY(Fact* SENS_MAG_XSCALE READ getSENS_MAG_XSCALE CONSTANT) Fact* getSENS_MAG_XSCALE(void) { return _mapParameterName2Fact["SENS_MAG_XSCALE"]; }
    Q_PROPERTY(Fact* SENS_MAG_YOFF READ getSENS_MAG_YOFF CONSTANT) Fact* getSENS_MAG_YOFF(void) { return _mapParameterName2Fact["SENS_MAG_YOFF"]; }
    Q_PROPERTY(Fact* SENS_MAG_YSCALE READ getSENS_MAG_YSCALE CONSTANT) Fact* getSENS_MAG_YSCALE(void) { return _mapParameterName2Fact["SENS_MAG_YSCALE"]; }
    Q_PROPERTY(Fact* SENS_MAG_ZOFF READ getSENS_MAG_ZOFF CONSTANT) Fact* getSENS_MAG_ZOFF(void) { return _mapParameterName2Fact["SENS_MAG_ZOFF"]; }
    Q_PROPERTY(Fact* SENS_MAG_ZSCALE READ getSENS_MAG_ZSCALE CONSTANT) Fact* getSENS_MAG_ZSCALE(void) { return _mapParameterName2Fact["SENS_MAG_ZSCALE"]; }
    Q_PROPERTY(Fact* SO3_COMP_KI READ getSO3_COMP_KI CONSTANT) Fact* getSO3_COMP_KI(void) { return _mapParameterName2Fact["SO3_COMP_KI"]; }
    Q_PROPERTY(Fact* SO3_COMP_KP READ getSO3_COMP_KP CONSTANT) Fact* getSO3_COMP_KP(void) { return _mapParameterName2Fact["SO3_COMP_KP"]; }
    Q_PROPERTY(Fact* SO3_PITCH_OFFS READ getSO3_PITCH_OFFS CONSTANT) Fact* getSO3_PITCH_OFFS(void) { return _mapParameterName2Fact["SO3_PITCH_OFFS"]; }
    Q_PROPERTY(Fact* SO3_ROLL_OFFS READ getSO3_ROLL_OFFS CONSTANT) Fact* getSO3_ROLL_OFFS(void) { return _mapParameterName2Fact["SO3_ROLL_OFFS"]; }
    Q_PROPERTY(Fact* SO3_YAW_OFFS READ getSO3_YAW_OFFS CONSTANT) Fact* getSO3_YAW_OFFS(void) { return _mapParameterName2Fact["SO3_YAW_OFFS"]; }
    Q_PROPERTY(Fact* SYS_AUTOCONFIG READ getSYS_AUTOCONFIG CONSTANT) Fact* getSYS_AUTOCONFIG(void) { return _mapParameterName2Fact["SYS_AUTOCONFIG"]; }
    Q_PROPERTY(Fact* SYS_AUTOSTART READ getSYS_AUTOSTART CONSTANT) Fact* getSYS_AUTOSTART(void) { return _mapParameterName2Fact["SYS_AUTOSTART"]; }
    Q_PROPERTY(Fact* SYS_RESTART_TYPE READ getSYS_RESTART_TYPE CONSTANT) Fact* getSYS_RESTART_TYPE(void) { return _mapParameterName2Fact["SYS_RESTART_TYPE"]; }
    Q_PROPERTY(Fact* SYS_USE_IO READ getSYS_USE_IO CONSTANT) Fact* getSYS_USE_IO(void) { return _mapParameterName2Fact["SYS_USE_IO"]; }
    Q_PROPERTY(Fact* TEST_D READ getTEST_D CONSTANT) Fact* getTEST_D(void) { return _mapParameterName2Fact["TEST_D"]; }
    Q_PROPERTY(Fact* TEST_DEV READ getTEST_DEV CONSTANT) Fact* getTEST_DEV(void) { return _mapParameterName2Fact["TEST_DEV"]; }
    Q_PROPERTY(Fact* TEST_D_LP READ getTEST_D_LP CONSTANT) Fact* getTEST_D_LP(void) { return _mapParameterName2Fact["TEST_D_LP"]; }
    Q_PROPERTY(Fact* TEST_HP READ getTEST_HP CONSTANT) Fact* getTEST_HP(void) { return _mapParameterName2Fact["TEST_HP"]; }
    Q_PROPERTY(Fact* TEST_I READ getTEST_I CONSTANT) Fact* getTEST_I(void) { return _mapParameterName2Fact["TEST_I"]; }
    Q_PROPERTY(Fact* TEST_I_MAX READ getTEST_I_MAX CONSTANT) Fact* getTEST_I_MAX(void) { return _mapParameterName2Fact["TEST_I_MAX"]; }
    Q_PROPERTY(Fact* TEST_LP READ getTEST_LP CONSTANT) Fact* getTEST_LP(void) { return _mapParameterName2Fact["TEST_LP"]; }
    Q_PROPERTY(Fact* TEST_MAX READ getTEST_MAX CONSTANT) Fact* getTEST_MAX(void) { return _mapParameterName2Fact["TEST_MAX"]; }
    Q_PROPERTY(Fact* TEST_MEAN READ getTEST_MEAN CONSTANT) Fact* getTEST_MEAN(void) { return _mapParameterName2Fact["TEST_MEAN"]; }
    Q_PROPERTY(Fact* TEST_MIN READ getTEST_MIN CONSTANT) Fact* getTEST_MIN(void) { return _mapParameterName2Fact["TEST_MIN"]; }
    Q_PROPERTY(Fact* TEST_P READ getTEST_P CONSTANT) Fact* getTEST_P(void) { return _mapParameterName2Fact["TEST_P"]; }
    Q_PROPERTY(Fact* TEST_TRIM READ getTEST_TRIM CONSTANT) Fact* getTEST_TRIM(void) { return _mapParameterName2Fact["TEST_TRIM"]; }
    Q_PROPERTY(Fact* TRIM_PITCH READ getTRIM_PITCH CONSTANT) Fact* getTRIM_PITCH(void) { return _mapParameterName2Fact["TRIM_PITCH"]; }
    Q_PROPERTY(Fact* TRIM_ROLL READ getTRIM_ROLL CONSTANT) Fact* getTRIM_ROLL(void) { return _mapParameterName2Fact["TRIM_ROLL"]; }
    Q_PROPERTY(Fact* TRIM_YAW READ getTRIM_YAW CONSTANT) Fact* getTRIM_YAW(void) { return _mapParameterName2Fact["TRIM_YAW"]; }
    Q_PROPERTY(Fact* UAVCAN_BITRATE READ getUAVCAN_BITRATE CONSTANT) Fact* getUAVCAN_BITRATE(void) { return _mapParameterName2Fact["UAVCAN_BITRATE"]; }
    Q_PROPERTY(Fact* UAVCAN_ENABLE READ getUAVCAN_ENABLE CONSTANT) Fact* getUAVCAN_ENABLE(void) { return _mapParameterName2Fact["UAVCAN_ENABLE"]; }
    Q_PROPERTY(Fact* UAVCAN_NODE_ID READ getUAVCAN_NODE_ID CONSTANT) Fact* getUAVCAN_NODE_ID(void) { return _mapParameterName2Fact["UAVCAN_NODE_ID"]; }
    Q_PROPERTY(QString testString READ getTestString CONSTANT)
    
public:
    /// @param uas Uas which this set of facts is associated with
    PX4ParameterFacts(UASInterface* uas, QObject* parent = NULL);
    
    ~PX4ParameterFacts();
    
    static void loadParameterFactMetaData(void);
    static void deleteParameterFactMetaData(void);
    static void clearStaticData(void);
    
    /// Returns true if the full set of facts are ready
    bool factsAreReady(void) { return _factsReady; }
    
signals:
    /// Signalled when the full set of facts are ready
    void factsReady(void);
    
private slots:
    void _parameterChanged(int uas, int component, QString parameterName, QVariant value);
    void _valueUpdated(QVariant value);
    void _paramMgrParameterListUpToDate(void);
    QString getTestString(void) { return QString("foo"); }
    
private:
    static FactMetaData* _parseParameter(QXmlStreamReader& xml, const QString& group);
    static void _initMetaData(FactMetaData* metaData);
    static QVariant _stringToTypedVariant(const QString& string, FactMetaData::ValueType_t type, bool failOk = false);

    QMap<QString, Fact*> _mapParameterName2Fact;    ///< Maps from a parameter name to a Fact
    QMap<Fact*, QString> _mapFact2ParameterName;    ///< Maps from a Fact to a parameter name
    
    static bool _parameterMetaDataLoaded;   ///< true: parameter meta data already loaded
    static QMap<QString, FactMetaData*> _mapParameterName2FactMetaData; ///< Maps from a parameter name to FactMetaData
    
    int _uasId;             ///< Id for uas which this set of Facts are associated with
    int _lastSeenComponent;
    
    QGCUASParamManagerInterface* _paramMgr;
    
    bool _factsReady;   ///< All facts received from param mgr
};

#endif