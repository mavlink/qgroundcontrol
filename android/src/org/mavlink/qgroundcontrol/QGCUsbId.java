package org.mavlink.qgroundcontrol;

public final class QGCUsbId {

    public static final int VENDOR_PX4 = 0x26AC;
    public static final int DEVICE_PX4FMU_V1 = 0x0010;
    public static final int DEVICE_PX4FMU_V2 = 0x0011;
    public static final int DEVICE_PX4FMU_V3 = 0x0011;
    public static final int DEVICE_PX4FMU_V4 = 0x0012;
    public static final int DEVICE_PX4FMU_V4PRO = 0x0013;
    public static final int DEVICE_PX4FMU_V5 = 0x0032;
    public static final int DEVICE_PX4FMU_V5X = 0x0033;
    public static final int DEVICE_PX4FMU_V6C = 0x0038;
    public static final int DEVICE_PX4FMU_V6U = 0x0036;
    public static final int DEVICE_PX4FMU_V6X = 0x0035;
    public static final int DEVICE_PX4FMU_V6XRT = 0x001D;
    public static final int DEVICE_PX4MINDPX_V2 = 0x0030;

    public static final int VENDOR_UBLOX = 0x1546;
    public static final int DEVICE_UBLOX_5 = 0x01a5;
    public static final int DEVICE_UBLOX_6 = 0x01a6;
    public static final int DEVICE_UBLOX_7 = 0x01a7;
    public static final int DEVICE_UBLOX_8 = 0x01a8;

    public static final int VENDOR_OPENPILOT = 0x20A0;
    public static final int DEVICE_REVOLUTION = 0x415E;
    public static final int DEVICE_OPLINK = 0x415C;
    public static final int DEVICE_SPARKY2 = 0x41D0;
    public static final int DEVICE_CC3D = 0x415D;

    public static final int VENDOR_STMICROELECTRONICS = 0x0483;
    public static final int VENDOR_ARDUPILOT = 0x1209;
    public static final int DEVICE_ARDUPILOT_CHIBIOS = 0x5740;
    public static final int DEVICE_ARDUPILOT_CHIBIOS2 = 0x5741;

    public static final int VENDOR_DRAGONLINK = 0x1FC9;
    public static final int DEVICE_DRAGONLINK = 0x0083;

    public static final int VENDOR_CUBEPILOT = 0x2DAE;
    public static final int DEVICE_CUBE_BLACK = 0x1011;
    public static final int DEVICE_CUBE_BLACK_BOOTLOADER = 0x1001;
    public static final int DEVICE_CUBE_BLACK_PLUS = 0x1011;
    public static final int DEVICE_CUBE_ORANGE = 0x1016;
    public static final int DEVICE_CUBE_ORANGE2 = 0x1017;
    public static final int DEVICE_CUBE_ORANGEPLUS = 0x1058;
    public static final int DEVICE_CUBE_YELLOW_BOOTLOADER = 0x1002;
    public static final int DEVICE_CUBE_YELLOW = 0x1012;
    public static final int DEVICE_CUBE_PURPLE_BOOTLOADER = 0x1005;
    public static final int DEVICE_CUBE_PURPLE = 0x1015;

    public static final int VENDOR_CUAV = 0x3163;
    public static final int DEVICE_CUAV_NORA = 0x004C;
    public static final int DEVICE_CUAV_X7PRO = 0x004C;

    public static final int VENDOR_HOLYBRO = 0x3162;
    public static final int DEVICE_PIXHAWK4 = 0x0047;
    public static final int DEVICE_PH4_MINI = 0x0049;
    public static final int DEVICE_DURANDAL = 0x004B;

    public static final int VENDOR_LASER_NAVIGATION = 0x27AC;
    public static final int DEVICE_VRBRAIN_V51 = 0x1151;
    public static final int DEVICE_VRBRAIN_V52 = 0x1152;
    public static final int DEVICE_VRBRAIN_V54 = 0x1154;
    public static final int DEVICE_VRCORE_V10 = 0x1910;
    public static final int DEVICE_VRUBRAIN_V51 = 0x1351;

    private QGCUsbId()
    {
        throw new IllegalAccessError("Non-instantiable class");
    }
}
