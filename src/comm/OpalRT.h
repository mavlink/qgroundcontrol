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
 *   @brief Types used for Opal-RT interface configuration
 *   @author Bryan Godbolt <godbolt@ualberta.ca>
 */

#ifndef OPALRT_H
#define OPALRT_H

#include <QString>
#include <QMessageBox>

#include "OpalApi.h"

namespace OpalRT
{
/**
  Configuration info for the model
 */

const unsigned short NUM_OUTPUT_SIGNALS=48;

/*  ------------------------------ Outputs ------------------------------
*
*      Port 1: Navigation state estimates
*      1       t   [s]     time elapsed since INS mode started
*      2-4     p^n [m]     navigation frame position (N,E,D)
*      5-7     v^n [m/s]   navigation frame velocity (N,E,D)
*      8-10    Euler angles [rad] (roll, pitch, yaw)
*     11-13    Angular rates
*     14-16    b_f [m/s^2] accelerometer biases
*     17-19    b_w [rad/s] gyro biases
*
*      Port 2: Navigation system status
*      1       mode    (0: initialization, 1: INS)
*      2       t_GPS   time elapsed since last valid GPS measurement
*      3       solution status (0: SOL_COMPUTED, 1: INSUFFICIENT_OBS)
*      4       position solution type ( 0: NONE, 34: NARROW_FLOAT,
*                                      49: WIDE_INT, 50: NARROW_INT)
*      5       # obs (number of visible satellites)
*
*      Port 3: Covariance matrix diagonal
*      1-15    diagonal elements of estimation error covariance matrix P
*/
enum SignalPort {
    T_ELAPSED,
    X_POS,
    Y_POS,
    Z_POS,
    X_VEL,
    Y_VEL,
    Z_VEL,
    ROLL,
    PITCH,
    YAW,
    ROLL_SPEED,
    PITCH_SPEED,
    YAW_SPEED,
    B_F_0,
    B_F_1,
    B_F_2,
    B_W_0,
    B_W_1,
    B_W_2,
    RAW_CHANNEL_1 = 24,
    RAW_CHANNEL_2,
    RAW_CHANNEL_3,
    RAW_CHANNEL_4,
    RAW_CHANNEL_5,
    RAW_CHANNEL_6,
    RAW_CHANNEL_7,
    RAW_CHANNEL_8,
    NORM_CHANNEL_1,
    NORM_CHANNEL_2,
    NORM_CHANNEL_3,
    NORM_CHANNEL_4,
    NORM_CHANNEL_5,
    NORM_CHANNEL_6,
    NORM_CHANNEL_7,
    NORM_CHANNEL_8,
    CONTROLLER_AILERON,
    CONTROLLER_ELEVATOR,
    AIL_POUT,
    AIL_IOUT,
    AIL_DOUT,
    ELE_POUT,
    ELE_IOUT,
    ELE_DOUT
};

/** Component IDs of the parameters.  Currently they are all 1 becuase there is no advantage
   to dividing them between component ids.  However this syntax is used so that this can
   easily be changed in the future.
   */
enum SubsystemIds {
    NAV = 1,
    LOG,
    CONTROLLER,
    SERVO_OUTPUTS,
    SERVO_INPUTS
};

class OpalErrorMsg
{
    static QString lastErrorMsg;
public:
    static void displayLastErrorMsg();
    static void setLastErrorMsg();
};
}
#endif // OPALRT_H
