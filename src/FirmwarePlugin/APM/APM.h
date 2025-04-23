/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

namespace APM
{

enum AUX_FUNC {
    DO_NOTHING =           0, // aux switch disabled
    FLIP =                 2, // flip
    SIMPLE_MODE =          3, // change to simple mode
    RTL =                  4, // change to RTL flight mode
    SAVE_TRIM =            5, // save current position as level
    SAVE_WP =              7, // save mission waypoint or RTL if in auto mode
    CAMERA_TRIGGER =       9, // trigger camera servo or relay
    RANGEFINDER =         10, // allow enabling or disabling rangefinder in flight which helps avoid surface tracking when you are far above the ground
    FENCE =               11, // allow enabling or disabling fence in flight
    RESETTOARMEDYAW =     12, // UNUSED
    SUPERSIMPLE_MODE =    13, // change to simple mode in middle, super simple at top
    ACRO_TRAINER =        14, // low = disabled, middle = leveled, high = leveled and limited
    SPRAYER =             15, // enable/disable the crop sprayer
    AUTO =                16, // change to auto flight mode
    AUTOTUNE_MODE =       17, // auto tune
    LAND =                18, // change to LAND flight mode
    GRIPPER =             19, // Operate cargo grippers low=off, middle=neutral, high=on
    PARACHUTE_ENABLE  =   21, // Parachute enable/disable
    PARACHUTE_RELEASE =   22, // Parachute release
    PARACHUTE_3POS =      23, // Parachute disable, enable, release with 3 position switch
    MISSION_RESET =       24, // Reset auto mission to start from first command
    ATTCON_FEEDFWD =      25, // enable/disable the roll and pitch rate feed forward
    ATTCON_ACCEL_LIM =    26, // enable/disable the roll, pitch and yaw accel limiting
    RETRACT_MOUNT1 =      27, // Retract Mount1
    RELAY =               28, // Relay pin on/off (only supports first relay)
    LANDING_GEAR =        29, // Landing gear controller
    LOST_VEHICLE_SOUND =  30, // Play lost vehicle sound
    MOTOR_ESTOP =         31, // Emergency Stop Switch
    MOTOR_INTERLOCK =     32, // Motor On/Off switch
    BRAKE =               33, // Brake flight mode
    RELAY2 =              34, // Relay2 pin on/off
    RELAY3 =              35, // Relay3 pin on/off
    RELAY4 =              36, // Relay4 pin on/off
    THROW =               37, // change to THROW flight mode
    AVOID_ADSB =          38, // enable AP_Avoidance library
    PRECISION_LOITER =    39, // enable precision loiter
    AVOID_PROXIMITY =     40, // enable object avoidance using proximity sensors (ie. horizontal lidar)
    ARMDISARM_UNUSED =    41, // UNUSED
    SMART_RTL =           42, // change to SmartRTL flight mode
    INVERTED  =           43, // enable inverted flight
    WINCH_ENABLE =        44, // winch enable/disable
    WINCH_CONTROL =       45, // winch control
    RC_OVERRIDE_ENABLE =  46, // enable RC Override
    USER_FUNC1 =          47, // user function #1
    USER_FUNC2 =          48, // user function #2
    USER_FUNC3 =          49, // user function #3
    LEARN_CRUISE =        50, // learn cruise throttle (Rover)
    MANUAL       =        51, // manual mode
    ACRO         =        52, // acro mode
    STEERING     =        53, // steering mode
    HOLD         =        54, // hold mode
    GUIDED       =        55, // guided mode
    LOITER       =        56, // loiter mode
    FOLLOW       =        57, // follow mode
    CLEAR_WP     =        58, // clear waypoints
    SIMPLE       =        59, // simple mode
    ZIGZAG       =        60, // zigzag mode
    ZIGZAG_SaveWP =       61, // zigzag save waypoint
    COMPASS_LEARN =       62, // learn compass offsets
    SAILBOAT_TACK =       63, // rover sailboat tack
    REVERSE_THROTTLE =    64, // reverse throttle input
    GPS_DISABLE  =        65, // disable GPS for testing
    RELAY5 =              66, // Relay5 pin on/off
    RELAY6 =              67, // Relay6 pin on/off
    STABILIZE =           68, // stabilize mode
    POSHOLD   =           69, // poshold mode
    ALTHOLD   =           70, // althold mode
    FLOWHOLD  =           71, // flowhold mode
    CIRCLE    =           72, // circle mode
    DRIFT     =           73, // drift mode
    SAILBOAT_MOTOR_3POS = 74, // Sailboat motoring 3pos
    SURFACE_TRACKING =    75, // Surface tracking upwards or downwards
    STANDBY  =            76, // Standby mode
    TAKEOFF   =           77, // takeoff
    RUNCAM_CONTROL =      78, // control RunCam device
    RUNCAM_OSD_CONTROL =  79, // control RunCam OSD
    VISODOM_ALIGN =       80, // align visual odometry camera's attitude to AHRS
    DISARM =              81, // disarm vehicle
    Q_ASSIST =            82, // disable, enable and force Q assist
    ZIGZAG_Auto =         83, // zigzag auto switch
    AIRMODE =             84, // enable / disable airmode for copter
    GENERATOR   =         85, // generator control
    TER_DISABLE =         86, // disable terrain following in CRUISE/FBWB modes
    CROW_SELECT =         87, // select CROW mode for diff spoilers;high disables,mid forces progressive
    SOARING =             88, // three-position switch to set soaring mode
    LANDING_FLARE =       89, // force flare, throttle forced idle, pitch to LAND_PITCH_DEG, tilts up
    EKF_SOURCE_SET =      90, // change EKF data source set between primary, secondary and tertiary
    ARSPD_CALIBRATE=      91, // calibrate airspeed ratio
    FBWA =                92, // Fly-By-Wire-A
    RELOCATE_MISSION =    93, // used in separate branch MISSION_RELATIVE
    VTX_POWER =           94, // VTX power level
    FBWA_TAILDRAGGER =    95, // enables FBWA taildragger takeoff mode. Once this feature is enabled it will stay enabled until the aircraft goes above TKOFF_TDRAG_SPD1 airspeed, changes mode, or the pitch goes above the initial pitch when this is engaged or goes below 0 pitch. When enabled the elevator will be forced to TKOFF_TDRAG_ELEV. This option allows for easier takeoffs on taildraggers in FBWA mode, and also makes it easier to test auto-takeoff steering handling in FBWA.
    MODE_SWITCH_RESET =   96, // trigger re-reading of mode switch
    WIND_VANE_DIR_OFSSET= 97, // flag for windvane direction offset input, used with windvane type 2
    TRAINING            = 98, // mode training
    AUTO_RTL =            99, // AUTO RTL via DO_LAND_START

    // entries from 100-150  are expected to be developer
    // options used for testing
    KILL_IMU1 =          100, // disable first IMU (for IMU failure testing)
    KILL_IMU2 =          101, // disable second IMU (for IMU failure testing)
    CAM_MODE_TOGGLE =    102, // Momentary switch to cycle camera modes
    EKF_LANE_SWITCH =    103, // trigger lane switch attempt
    EKF_YAW_RESET =      104, // trigger yaw reset attempt
    GPS_DISABLE_YAW =    105, // disable GPS yaw for testing
    DISABLE_AIRSPEED_USE = 106, // equivalent to AIRSPEED_USE 0
    FW_AUTOTUNE =          107, // fixed wing auto tune
    QRTL =               108, // QRTL mode
    CUSTOM_CONTROLLER =  109,  // use Custom Controller
    KILL_IMU3 =          110, // disable third IMU (for IMU failure testing)
    LOWEHEISER_STARTER = 111,  // allows for manually running starter
    AHRS_TYPE =          112, // change AHRS_EKF_TYPE
    RETRACT_MOUNT2 =     113, // Retract Mount2

    // if you add something here, make sure to update the documentation of the parameter in RC_Channel.cpp!
    // also, if you add an option >255, you will need to fix duplicate_options_exist

    // options 150-199 continue user rc switch options
    CRUISE =             150,  // CRUISE mode
    TURTLE =             151,  // Turtle mode - flip over after crash
    SIMPLE_HEADING_RESET = 152, // reset simple mode reference heading to current
    ARMDISARM =          153, // arm or disarm vehicle
    ARMDISARM_AIRMODE =  154, // arm or disarm vehicle enabling airmode
    TRIM_TO_CURRENT_SERVO_RC = 155, // trim to current servo and RC
    TORQEEDO_CLEAR_ERR = 156, // clear torqeedo error
    EMERGENCY_LANDING_EN = 157, //Force long FS action to FBWA for landing out of range
    OPTFLOW_CAL =        158, // optical flow calibration
    FORCEFLYING =        159, // enable or disable land detection for GPS based manual modes preventing land detection and maintainting set_throttle_mix_max
    WEATHER_VANE_ENABLE = 160, // enable/disable weathervaning
    TURBINE_START =      161, // initialize turbine start sequence
    FFT_NOTCH_TUNE =     162, // FFT notch tuning function
    MOUNT_LOCK =         163, // Mount yaw lock vs follow
    LOG_PAUSE =          164, // Pauses logging if under logging rate control
    ARM_EMERGENCY_STOP = 165, // ARM on high, MOTOR_ESTOP on low
    CAMERA_REC_VIDEO =   166, // start recording on high, stop recording on low
    CAMERA_ZOOM =        167, // camera zoom high = zoom in, middle = hold, low = zoom out
    CAMERA_MANUAL_FOCUS = 168,// camera manual focus.  high = long shot, middle = stop focus, low = close shot
    CAMERA_AUTO_FOCUS =  169, // camera auto focus
    QSTABILIZE =         170, // QuadPlane QStabilize mode
    MAG_CAL =            171, // Calibrate compasses (disarmed only)
    BATTERY_MPPT_ENABLE = 172,// Battery MPPT Power enable. high = ON, mid = auto (controlled by mppt/batt driver), low = OFF. This effects all MPPTs.
    PLANE_AUTO_LANDING_ABORT = 173, // Abort Glide-slope or VTOL landing during payload place or do_land type mission items
    CAMERA_IMAGE_TRACKING = 174, // camera image tracking
    CAMERA_LENS =        175, // camera lens selection
    VFWD_THR_OVERRIDE =  176, // force enabled VTOL forward throttle method
    MOUNT_LRF_ENABLE =   177,  // mount LRF enable/disable
    FLIGHTMODE_PAUSE =   178,  // e.g. pause movement towards waypoint
    ICE_START_STOP =     179, // AP_ICEngine start stop
    AUTOTUNE_TEST_GAINS = 180, // auto tune tuning switch to test or revert gains
    QUICKTUNE =          181,  //quicktune 3 position switch
    AHRS_AUTO_TRIM =     182,  // in-flight AHRS autotrim
    AUTOLAND =           183,  //Fixed Wing AUTOLAND Mode
    SYSTEMID =           184,  // system ID as an aux switch

    // inputs from 200 will eventually used to replace RCMAP
    ROLL =               201, // roll input
    PITCH =              202, // pitch input
    THROTTLE =           203, // throttle pilot input
    YAW =                204, // yaw pilot input
    MAINSAIL =           207, // mainsail input
    FLAP =               208, // flap input
    FWD_THR =            209, // VTOL manual forward throttle
    AIRBRAKE =           210, // manual airbrake control
    WALKING_HEIGHT =     211, // walking robot height input
    MOUNT1_ROLL =        212, // mount1 roll input
    MOUNT1_PITCH =       213, // mount1 pitch input
    MOUNT1_YAW =         214, // mount1 yaw input
    MOUNT2_ROLL =        215, // mount2 roll input
    MOUNT2_PITCH =       216, // mount3 pitch input
    MOUNT2_YAW =         217, // mount4 yaw input
    LOWEHEISER_THROTTLE= 218, // allows for throttle on slider
    TRANSMITTER_TUNING = 219, // use a transmitter knob or slider for in-flight tuning

    // inputs 248-249 are reserved for the Skybrush fork at
    // https://github.com/skybrush-io/ardupilot

    // inputs for the use of onboard lua scripting
    SCRIPTING_1 =        300,
    SCRIPTING_2 =        301,
    SCRIPTING_3 =        302,
    SCRIPTING_4 =        303,
    SCRIPTING_5 =        304,
    SCRIPTING_6 =        305,
    SCRIPTING_7 =        306,
    SCRIPTING_8 =        307,
    SCRIPTING_9 =        308,
    SCRIPTING_10 =       309,
    SCRIPTING_11 =       310,
    SCRIPTING_12 =       311,
    SCRIPTING_13 =       312,
    SCRIPTING_14 =       313,
    SCRIPTING_15 =       314,
    SCRIPTING_16 =       315,

    // this must be higher than any aux function above
    AUX_FUNCTION_MAX =   316,
};

} // namespace APM
