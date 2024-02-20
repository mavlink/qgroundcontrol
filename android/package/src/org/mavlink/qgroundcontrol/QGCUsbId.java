package org.mavlink.qgroundcontrol;

public final class QGCUsbId {

    public static final int VENDOR_PX4 = 0x26AC;
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

    public static final int VENDOR_ATMEL = 0x03EB;
    public static final int DEVICE_ATMEL_LUFA_CDC_DEMO_APP = 0x2044;

    public static final int VENDOR_ARDUINO = 0x2341;
    public static final int DEVICE_ARDUINO_UNO = 0x0001;
    public static final int DEVICE_ARDUINO_MEGA_2560 = 0x0010;
    public static final int DEVICE_ARDUINO_SERIAL_ADAPTER = 0x003b;
    public static final int DEVICE_ARDUINO_MEGA_ADK = 0x003f;
    public static final int DEVICE_ARDUINO_MEGA_2560_R3 = 0x0042;
    public static final int DEVICE_ARDUINO_UNO_R3 = 0x0043;
    public static final int DEVICE_ARDUINO_MEGA_ADK_R3 = 0x0044;
    public static final int DEVICE_ARDUINO_SERIAL_ADAPTER_R3 = 0x0044;
    public static final int DEVICE_ARDUINO_LEONARDO = 0x8036;

    public static final int VENDOR_VAN_OOIJEN_TECH = 0x16c0;
    public static final int DEVICE_VAN_OOIJEN_TECH_TEENSYDUINO_SERIAL = 0x0483;

    public static final int VENDOR_LEAFLABS = 0x1eaf;
    public static final int DEVICE_LEAFLABS_MAPLE = 0x0004;

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

    public static final int VENDOR_ARDUPILOT_CHIBIOS1 = 0x0483;
    public static final int VENDOR_ARDUPILOT_CHIBIOS2 = 0x1209;
    public static final int DEVICE_ARDUPILOT_CHIBIOS = 0x5740;

    public static final int VENDOR_DRAGONLINK = 0x1FC9;
    public static final int DEVICE_DRAGONLINK = 0x0083;

    public static final int VENDOR_CUBEPILOT = 0x2DAE;
    public static final int DEVICE_CUBE_ORANGE = 0x1016;
    public static final int DEVICE_CUBE_ORANGEPLUS = 0x1058;
    public static final int DEVICE_CUBE_YELLOW = 0x1012;

    public static final int VENDOR_CUAV = 0x3163;
    public static final int DEVICE_CUAV_NORA = 0x004C;
    public static final int DEVICE_CUAV_X7PRO = 0x004C;


    private UsbId()
    {
        throw new IllegalAccessError("Non-instantiable class");
    }
}
