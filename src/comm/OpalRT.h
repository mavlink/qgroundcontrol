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

#ifndef OPALRT_H
#define OPALRT_H

namespace OpalRT
{
    /*  ------------------------------ Outputs ------------------------------
    *
    *    Copied from Mag_GPS_aided_INS.c Aug 20, 2010
    *      Port 1: Navigation state estimates
    *      1       t   [s]     time elapsed since INS mode started
    *      2-4     p^n [m]     navigation frame position (N,E,D)
    *      5-7     v^n [m/s]   navigation frame velocity (N,E,D)
    *      8-10    Euler angles [rad] (roll, pitch, yaw)
    *     11-13    b_f [m/s^2] accelerometer biases
    *     14-16    b_w [rad/s] gyro biases
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
    enum SignalPort
    {
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
        B_F_0,
        B_F_1,
        B_F_2,
        B_W_0,
        B_W_1,
        B_W_2
    };       

    /* Component IDs of the parameters.  Currently they are all 1 becuase there is no advantage
       to dividing them between component ids.  However this syntax is used so that this can
       easily be changed in the future.
       */
    enum SubsystemIds
    {
        NAV_ID = 1,
        LOG_ID = 1,
        CONTROLLER_ID = 1
    };
}
#endif // OPALRT_H
