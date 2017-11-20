/*!
 * @file
 * @brief ST16 Enums ported from original Java code
 * @author Gus Grubba <mavlink@grubba.com>
 *
 */

#pragma once

namespace Yuneec {

enum {
    /**
     * The length of command body excluding data
     */
        COMMAND_BODY_EXCLUDE_VALUES_LENGTH = 10,
    /**
     * Command ID of {@link com.yuneec.droneservice.command.common.EnterBind}
     * <p/>
     * Value is {@value}
     */
        CMD_ENTER_BIND = 0x66,
    /**
     * Command ID of {@link com.yuneec.droneservice.command.common.StartBind}
     * <p/>
     * Value is {@value}
     */
        CMD_START_BIND = 2,
    /**
     * Command ID of {@link com.yuneec.droneservice.command.common.Bind}
     * <p/>
     * Value is {@value}
     */
        CMD_BIND = 4,
    /**
     * Command ID of {@link com.yuneec.droneservice.command.common.QueryBindState}
     * <p/>
     * Value is {@value}
     */
        CMD_QUERY_BIND_STATE = 0x87,
    /**
     * Command ID of {@link com.yuneec.droneservice.command.common.SetChannelSetting}
     * <p/>
     * Value is {@value}
     */
        CMD_SET_CHANNEL_SETTING = 0x95,
    /**
     * Command ID of {@link com.yuneec.droneservice.command.common.SyncMixingDataDeleteAll}
     * <p/>
     * Value is {@value}
     */
        CMD_SYNC_MIXING_DATA_DELETE_ALL = 0xA3,
    /**
     * Command ID of {@link com.yuneec.droneservice.command.common.SyncMixingDataAdd}
     * <p/>
     * Value is {@value}
     */
        CMD_SYNC_MIXING_DATA_ADD = 0x79,
    /**
     * Command ID of {@link com.yuneec.droneservice.command.common.SendRxResInfo}
     * <p/>
     * Value is {@value}
     */
        CMD_SEND_RX_RESINFO = 0x65,
    /**
     * Command ID of {@link com.yuneec.droneservice.command.common.EnterRun}
     * <p/>
     * Value is {@value}
     */
        CMD_ENTER_RUN = 0x68,
    /**
     * Command ID of {@link com.yuneec.droneservice.command.common.ExitRun}
     * <p/>
     * Value is {@value}
     */
        CMD_EXIT_RUN = 0x69,
    /**
     * Command ID of {@link com.yuneec.droneservice.command.common.ExitBind}
     * <p/>
     * Value is {@value}
     */
        CMD_EXIT_BIND = 0x67,
    /**
     * Command ID of {@link com.yuneec.droneservice.command.common.UnBind}
     * <p/>
     * Value is {@value}
     */
        CMD_UNBIND = 0x86,
    /**
     * Command ID of {@link com.yuneec.droneservice.command.common.RecvBothCh}
     * <p/>
     * Value is {@value}
     */
        CMD_RECV_BOTH_CH = 0x76,
    /**
     * Command ID of {@link com.yuneec.droneservice.command.common.RecvMixedChOnly}
     * <p/>
     * Value is {@value}
     */
        CMD_RECV_MIXED_CH_ONLY = 0x75,
    /**
     * Command ID of {@link com.yuneec.droneservice.command.common.PowerKey}
     * <p/>
     * Value is {@value}
     */
        CMD_SET_BINDKEY_FUNCTION = 0x91,
    /**
     * Command ID of {@link com.yuneec.droneservice.command.common.UpdateTxCompleted}
     * <p/>
     * Value is {@value}
     */
        CMD_UPDATE_TX_COMPLETED = 0x9b,


        CMD_EXIT_TO_AWAIT = 0xBD,

    /**
     * Received data type：bind
     * <p/>
     * Value is {@value}
     */
        TYPE_BIND = 0,
    /**
     * Received data type：channel
     * <p/>
     * Value is {@value}
     */
        TYPE_CHN = 1,
    /**
     * Received data type：command
     * <p/>
     * Value is {@value}
     */
        TYPE_CMD = 3,
    /**
     * Received data type：response
     * <p/>
     * Value is {@value}
     */
        TYPE_RSP = 4,
    /**
     * Received data type： pass through, it contains mission type.
     * <p/>
     * Value is {@value}
     */
        TYPE_PASS_THROUGH = 5,

    /**
     * Mission status:run,received from aircraft.
     * <p/>
     * Value is {@value}
     */
        MISSIONSTATUS_RUN = 1,
    /**
     * Mission status:pause,received from aircraft.
     * <p/>
     * Value is {@value}
     */
        MISSIONSTATUS_PAUSE = 2,
    /**
     * Mission status:exit,received from aircraft.
     * <p/>
     * Value is {@value}
     */
        MISSIONSTATUS_EXIT = 0,
    /**
     * Default command type
     * <p/>
     * Value is {@value}
     */
        COMMAND_TYPE_NORMAL = (TYPE_CMD << 2),
    /**
     * Mission type of {@link com.yuneec.droneservice.command.common.LandingRequest},
     * {@link SetWaypointRequest},
     * {@link com.yuneec.droneservice.command.common.TakeOffRequest},
     * {@link com.yuneec.droneservice.command.common.WaypointConfigRequest},
     * {@link com.yuneec.droneservice.command.common.WayPointRequest}
     * <p/>
     * Value is {@value}
     */
        COMMAND_TYPE_PASS_THROUGH = (TYPE_PASS_THROUGH << 2),

    /**
     * Parameter of {@link com.yuneec.droneservice.command.common.PowerKey},represent the function
     * of power key is working.For example,when you click power key,the screen will light up or
     * go out.
     * <p/>
     * Value is {@value}
     */
        BIND_KEY_FUNCTION_PWR = 0,
    /**
     * Parameter of {@link com.yuneec.droneservice.command.common.PowerKey},represent the function
     * of power key is not working.
     * <p/>
     * Value is {@value}
     */
        BIND_KEY_FUNCTION_BIND = 1,

    /**
     * Default sub-ID of command
     * <p/>
     * Value is {@value}
     */
        DEFAULT_SUBCOMMAND_ID = -1,

    /**
     * actionType of {@link com.yuneec.droneservice.parse.MissionFeedback},
     * <p/>
     * Value is {@value}
     */
        ACTION_TYPE_NONE = 255,

    /**
     * Command id of {@link com.yuneec.droneservice.command.common.MapConfigRequest},
     * {@link MapSetWaypointRequest},
     * {@link com.yuneec.droneservice.command.common.MapWaypointRequest}
     * <p/>
     * Value is {@value}
     */
        ACTION_TYPE_MAP_WAYPOINT = 12,
    /**
     * Command ID of {@link FenceConfigRequest},
     * {@link com.yuneec.droneservice.command.common.FenceRequest},
     * {@link FenceSetWaypointRequest}
     * <p/>
     * Value is {@value}
     */
        ACTION_TYPE_FENCE = 13,
    /**
     * Command ID of {@link com.yuneec.droneservice.command.common.LandingRequest}
     * <p/>
     * Value is {@value}
     */
        ACTION_TYPE_LANDING = 14,
    /**
     * Command ID of {@link com.yuneec.droneservice.command.common.TakeOffRequest}
     * <p/>
     * Value is {@value}
     */
        ACTION_TYPE_TAKEOFF = 15,

    /**
     * Request type of way point:start
     * <p/>
     * Value is {@value}
     */
        ACTION_REQUEST_START = 0,
    /**
     * Request type of way point:pause
     * <p/>
     * Value is {@value}
     */
        ACTION_REQUEST_PAUSE = 2,
    /**
     * Request type of way point:resume
     * <p/>
     * Value is {@value}
     */
        ACTION_REQUEST_RESUME = 3,
    /**
     * Request type of way point:exit
     * <p/>
     * Value is {@value}
     */
        ACTION_REQUEST_EXIT = 4,
    /**
     * Request type of way point:：ask to return position to GCS
     * <p/>
     * Value is {@value}
     */
        ACTION_REQUEST_GET = 5,
    /**
     * Request type of way point:：sending wp data to MAV, in a list way
     * <p/>
     * Value is {@value}
     */
        ACTION_REQUEST_SET = 6,
    /**
     * Request type of way point:：configuration
     * <p/>
     * Value is {@value}
     */
        ACTION_REQUEST_CONFIG = 8,


        CMD_ENTER_FACTORY_CAL           = 0x9F,
        CMD_EXIT_FACTORY_CAL            = 0xA0,
        CMD_CALIBRATION_STATE_CHANGE    = 0x96,

        CMD_TX_CHANNEL_DATA_RAW         = 0x77,
        CMD_TX_CHANNEL_DATA_MIXED       = 0x78,
        CMD_TX_STATE_MACHINE            = 0x83,
        CMD_RX_FEEDBACK_DATA            = 0x88,
        CMD_TX_SWITCH_CHANGED           = 0x89,
        COMMAND_M4_SEND_GPS_DATA_TO_PA  = 0xB6,
        CMD_SET_TTB_STATE               = 0x92,
        ACTION_TYPE_RESPONSE            = 1,
        ACTION_TYPE_FEEDBACK            = 3,
        ACTION_TYPE_ONEKEY_TAKEOFF      = 6,

        BUTTON_OBS                      = 33,
        BUTTON_POWER                    = 51,
        BUTTON_CAMERA_SHUTTER           = 53,
        BUTTON_VIDEO_SHUTTER            = 54,

};

}
