/** @file
 *  @brief MAVLink comm protocol generated from ardupilotmega.xml
 *  @see http://mavlink.org
 */
#pragma once
#ifndef MAVLINK_ARDUPILOTMEGA_H
#define MAVLINK_ARDUPILOTMEGA_H

#ifndef MAVLINK_H
    #error Wrong include order: MAVLINK_ARDUPILOTMEGA.H MUST NOT BE DIRECTLY USED. Include mavlink.h from the same directory instead or set ALL AND EVERY defines from MAVLINK.H manually accordingly, including the #define MAVLINK_H call.
#endif

#define MAVLINK_ARDUPILOTMEGA_XML_HASH -1894036221487711500

#ifdef __cplusplus
extern "C" {
#endif

// MESSAGE LENGTHS AND CRCS

#ifndef MAVLINK_MESSAGE_LENGTHS
#define MAVLINK_MESSAGE_LENGTHS {}
#endif

#ifndef MAVLINK_MESSAGE_CRCS
#define MAVLINK_MESSAGE_CRCS {{0, 50, 9, 9, 0, 0, 0}, {1, 124, 31, 43, 0, 0, 0}, {2, 137, 12, 12, 0, 0, 0}, {4, 237, 14, 14, 3, 12, 13}, {5, 217, 28, 28, 1, 0, 0}, {6, 104, 3, 3, 0, 0, 0}, {7, 119, 32, 32, 0, 0, 0}, {8, 117, 36, 36, 0, 0, 0}, {11, 89, 6, 6, 1, 4, 0}, {20, 214, 20, 20, 3, 2, 3}, {21, 159, 2, 2, 3, 0, 1}, {22, 220, 25, 25, 0, 0, 0}, {23, 168, 23, 23, 3, 4, 5}, {24, 24, 30, 52, 0, 0, 0}, {25, 23, 101, 101, 0, 0, 0}, {26, 170, 22, 24, 0, 0, 0}, {27, 144, 26, 29, 0, 0, 0}, {28, 67, 16, 16, 0, 0, 0}, {29, 115, 14, 16, 0, 0, 0}, {30, 39, 28, 28, 0, 0, 0}, {31, 246, 32, 48, 0, 0, 0}, {32, 185, 28, 28, 0, 0, 0}, {33, 104, 28, 28, 0, 0, 0}, {34, 237, 22, 22, 0, 0, 0}, {35, 244, 22, 22, 0, 0, 0}, {36, 222, 21, 37, 0, 0, 0}, {37, 212, 6, 7, 3, 4, 5}, {38, 9, 6, 7, 3, 4, 5}, {39, 254, 37, 38, 3, 32, 33}, {40, 230, 4, 5, 3, 2, 3}, {41, 28, 4, 4, 3, 2, 3}, {42, 28, 2, 18, 0, 0, 0}, {43, 132, 2, 3, 3, 0, 1}, {44, 221, 4, 9, 3, 2, 3}, {45, 232, 2, 3, 3, 0, 1}, {46, 11, 2, 2, 0, 0, 0}, {47, 153, 3, 8, 3, 0, 1}, {48, 41, 13, 21, 1, 12, 0}, {49, 39, 12, 20, 0, 0, 0}, {50, 78, 37, 37, 3, 18, 19}, {51, 196, 4, 5, 3, 2, 3}, {54, 15, 27, 27, 3, 24, 25}, {55, 3, 25, 25, 0, 0, 0}, {61, 167, 72, 72, 0, 0, 0}, {62, 183, 26, 26, 0, 0, 0}, {63, 119, 181, 181, 0, 0, 0}, {64, 191, 225, 225, 0, 0, 0}, {65, 118, 42, 42, 0, 0, 0}, {66, 148, 6, 6, 3, 2, 3}, {67, 21, 4, 4, 0, 0, 0}, {69, 243, 11, 30, 1, 10, 0}, {70, 124, 18, 38, 3, 16, 17}, {73, 38, 37, 38, 3, 32, 33}, {74, 20, 20, 20, 0, 0, 0}, {75, 158, 35, 35, 3, 30, 31}, {76, 152, 33, 33, 3, 30, 31}, {77, 143, 3, 10, 3, 8, 9}, {80, 14, 4, 4, 3, 2, 3}, {81, 106, 22, 22, 0, 0, 0}, {82, 49, 39, 51, 3, 36, 37}, {83, 22, 37, 37, 0, 0, 0}, {84, 143, 53, 53, 3, 50, 51}, {85, 140, 51, 51, 0, 0, 0}, {86, 5, 53, 53, 3, 50, 51}, {87, 150, 51, 51, 0, 0, 0}, {89, 231, 28, 28, 0, 0, 0}, {90, 183, 56, 56, 0, 0, 0}, {91, 63, 42, 42, 0, 0, 0}, {92, 54, 33, 33, 0, 0, 0}, {93, 47, 81, 81, 0, 0, 0}, {100, 175, 26, 34, 0, 0, 0}, {101, 102, 32, 117, 0, 0, 0}, {102, 158, 32, 117, 0, 0, 0}, {103, 208, 20, 57, 0, 0, 0}, {104, 56, 32, 116, 0, 0, 0}, {105, 93, 62, 63, 0, 0, 0}, {106, 138, 44, 44, 0, 0, 0}, {107, 108, 64, 65, 0, 0, 0}, {108, 32, 84, 92, 0, 0, 0}, {109, 185, 9, 9, 0, 0, 0}, {110, 84, 254, 254, 3, 1, 2}, {111, 34, 16, 18, 3, 16, 17}, {112, 174, 12, 12, 0, 0, 0}, {113, 124, 36, 39, 0, 0, 0}, {114, 237, 44, 44, 0, 0, 0}, {115, 4, 64, 64, 0, 0, 0}, {116, 76, 22, 24, 0, 0, 0}, {117, 128, 6, 6, 3, 4, 5}, {118, 56, 14, 14, 0, 0, 0}, {119, 116, 12, 12, 3, 10, 11}, {120, 134, 97, 97, 0, 0, 0}, {121, 237, 2, 2, 3, 0, 1}, {122, 203, 2, 2, 3, 0, 1}, {123, 250, 113, 113, 3, 0, 1}, {124, 87, 35, 57, 0, 0, 0}, {125, 203, 6, 6, 0, 0, 0}, {126, 220, 79, 81, 3, 79, 80}, {127, 25, 35, 35, 0, 0, 0}, {128, 226, 35, 35, 0, 0, 0}, {129, 46, 22, 24, 0, 0, 0}, {130, 29, 13, 13, 0, 0, 0}, {131, 223, 255, 255, 0, 0, 0}, {132, 85, 14, 39, 0, 0, 0}, {133, 6, 18, 18, 0, 0, 0}, {134, 229, 43, 43, 0, 0, 0}, {135, 203, 8, 8, 0, 0, 0}, {136, 1, 22, 22, 0, 0, 0}, {137, 195, 14, 16, 0, 0, 0}, {138, 109, 36, 120, 0, 0, 0}, {139, 168, 43, 43, 3, 41, 42}, {140, 181, 41, 41, 0, 0, 0}, {141, 47, 32, 32, 0, 0, 0}, {142, 72, 243, 243, 0, 0, 0}, {143, 131, 14, 16, 0, 0, 0}, {144, 127, 93, 93, 0, 0, 0}, {146, 103, 100, 100, 0, 0, 0}, {147, 154, 36, 54, 0, 0, 0}, {148, 178, 60, 78, 0, 0, 0}, {149, 200, 30, 60, 0, 0, 0}, {150, 134, 42, 42, 0, 0, 0}, {151, 219, 8, 8, 3, 6, 7}, {152, 208, 4, 8, 0, 0, 0}, {153, 188, 12, 12, 0, 0, 0}, {154, 84, 15, 15, 3, 6, 7}, {155, 22, 13, 13, 3, 4, 5}, {156, 19, 6, 6, 3, 0, 1}, {157, 21, 15, 15, 3, 12, 13}, {158, 134, 14, 15, 3, 12, 13}, {160, 78, 12, 12, 3, 8, 9}, {161, 68, 3, 3, 3, 0, 1}, {162, 189, 8, 9, 0, 0, 0}, {163, 127, 28, 28, 0, 0, 0}, {164, 154, 44, 44, 0, 0, 0}, {165, 21, 3, 3, 0, 0, 0}, {166, 21, 9, 9, 0, 0, 0}, {167, 144, 22, 22, 0, 0, 0}, {168, 1, 12, 12, 0, 0, 0}, {169, 234, 18, 18, 0, 0, 0}, {170, 73, 34, 34, 0, 0, 0}, {171, 181, 66, 66, 0, 0, 0}, {172, 22, 98, 98, 0, 0, 0}, {173, 83, 8, 8, 0, 0, 0}, {174, 167, 48, 48, 0, 0, 0}, {175, 138, 19, 19, 3, 14, 15}, {176, 234, 3, 3, 3, 0, 1}, {177, 240, 20, 20, 0, 0, 0}, {178, 47, 24, 24, 0, 0, 0}, {179, 189, 29, 29, 1, 26, 0}, {180, 52, 45, 47, 1, 42, 0}, {181, 174, 4, 4, 0, 0, 0}, {182, 229, 40, 40, 0, 0, 0}, {183, 85, 2, 2, 3, 0, 1}, {184, 159, 206, 206, 3, 4, 5}, {185, 186, 7, 7, 3, 4, 5}, {186, 72, 29, 29, 3, 0, 1}, {191, 92, 27, 27, 0, 0, 0}, {192, 36, 44, 54, 0, 0, 0}, {193, 71, 22, 26, 0, 0, 0}, {194, 98, 25, 33, 0, 0, 0}, {195, 120, 37, 37, 0, 0, 0}, {200, 134, 42, 42, 3, 40, 41}, {201, 205, 14, 14, 3, 12, 13}, {214, 69, 8, 8, 3, 6, 7}, {215, 101, 3, 3, 0, 0, 0}, {216, 50, 3, 3, 3, 0, 1}, {217, 202, 6, 6, 0, 0, 0}, {218, 17, 7, 7, 3, 0, 1}, {219, 162, 2, 2, 0, 0, 0}, {225, 208, 65, 73, 0, 0, 0}, {226, 207, 8, 8, 0, 0, 0}, {230, 163, 42, 42, 0, 0, 0}, {231, 105, 40, 40, 0, 0, 0}, {232, 151, 63, 65, 0, 0, 0}, {233, 35, 182, 182, 0, 0, 0}, {234, 150, 40, 40, 0, 0, 0}, {235, 179, 42, 42, 0, 0, 0}, {241, 90, 32, 32, 0, 0, 0}, {242, 104, 52, 60, 0, 0, 0}, {243, 85, 53, 61, 1, 52, 0}, {244, 95, 6, 6, 0, 0, 0}, {245, 130, 2, 2, 0, 0, 0}, {246, 184, 38, 38, 0, 0, 0}, {247, 81, 19, 19, 0, 0, 0}, {248, 8, 254, 254, 3, 3, 4}, {249, 204, 36, 36, 0, 0, 0}, {250, 49, 30, 30, 0, 0, 0}, {251, 170, 18, 18, 0, 0, 0}, {252, 44, 18, 18, 0, 0, 0}, {253, 83, 51, 54, 0, 0, 0}, {254, 46, 9, 9, 0, 0, 0}, {256, 71, 42, 42, 3, 8, 9}, {257, 131, 9, 9, 0, 0, 0}, {258, 187, 32, 232, 3, 0, 1}, {259, 92, 235, 237, 0, 0, 0}, {260, 146, 5, 14, 0, 0, 0}, {261, 179, 27, 61, 0, 0, 0}, {262, 12, 18, 23, 0, 0, 0}, {263, 133, 255, 255, 0, 0, 0}, {264, 49, 28, 32, 0, 0, 0}, {265, 26, 16, 20, 0, 0, 0}, {266, 193, 255, 255, 3, 2, 3}, {267, 35, 255, 255, 3, 2, 3}, {268, 14, 4, 4, 3, 2, 3}, {269, 109, 213, 215, 0, 0, 0}, {270, 59, 19, 20, 0, 0, 0}, {271, 22, 52, 53, 0, 0, 0}, {275, 126, 31, 32, 0, 0, 0}, {276, 18, 49, 50, 0, 0, 0}, {277, 62, 30, 30, 0, 0, 0}, {280, 70, 33, 33, 0, 0, 0}, {281, 48, 13, 13, 0, 0, 0}, {282, 123, 35, 35, 3, 32, 33}, {283, 74, 144, 145, 0, 0, 0}, {284, 99, 32, 32, 3, 30, 31}, {285, 137, 40, 49, 3, 38, 39}, {286, 210, 53, 57, 3, 50, 51}, {287, 1, 23, 23, 3, 20, 21}, {288, 20, 23, 23, 3, 20, 21}, {290, 251, 46, 46, 0, 0, 0}, {291, 10, 57, 57, 0, 0, 0}, {295, 234, 12, 12, 0, 0, 0}, {299, 19, 96, 98, 0, 0, 0}, {300, 217, 22, 22, 0, 0, 0}, {301, 243, 58, 58, 0, 0, 0}, {310, 28, 17, 17, 0, 0, 0}, {311, 95, 116, 116, 0, 0, 0}, {320, 243, 20, 20, 3, 2, 3}, {321, 88, 2, 2, 3, 0, 1}, {322, 243, 149, 149, 0, 0, 0}, {323, 78, 147, 147, 3, 0, 1}, {324, 132, 146, 146, 0, 0, 0}, {330, 23, 158, 167, 0, 0, 0}, {331, 91, 230, 233, 0, 0, 0}, {332, 236, 239, 239, 0, 0, 0}, {333, 231, 109, 109, 0, 0, 0}, {334, 72, 10, 10, 0, 0, 0}, {335, 225, 24, 24, 0, 0, 0}, {336, 245, 84, 84, 0, 0, 0}, {339, 199, 5, 5, 0, 0, 0}, {340, 99, 70, 70, 0, 0, 0}, {345, 209, 21, 21, 3, 2, 3}, {350, 232, 20, 252, 0, 0, 0}, {360, 11, 25, 25, 0, 0, 0}, {361, 93, 33, 33, 0, 0, 0}, {370, 75, 87, 109, 0, 0, 0}, {371, 10, 26, 26, 0, 0, 0}, {372, 26, 140, 140, 0, 0, 0}, {373, 117, 42, 42, 0, 0, 0}, {375, 251, 140, 140, 0, 0, 0}, {380, 232, 20, 20, 0, 0, 0}, {385, 147, 133, 133, 3, 2, 3}, {386, 132, 16, 16, 3, 4, 5}, {387, 4, 72, 72, 3, 4, 5}, {388, 8, 37, 37, 3, 32, 33}, {390, 156, 238, 240, 0, 0, 0}, {395, 0, 212, 212, 0, 0, 0}, {396, 50, 160, 160, 0, 0, 0}, {397, 182, 108, 108, 0, 0, 0}, {400, 110, 254, 254, 3, 4, 5}, {401, 183, 6, 6, 3, 4, 5}, {410, 160, 53, 53, 0, 0, 0}, {411, 106, 3, 3, 0, 0, 0}, {412, 33, 6, 6, 3, 4, 5}, {413, 77, 7, 7, 3, 4, 5}, {435, 134, 46, 46, 0, 0, 0}, {436, 193, 9, 9, 0, 0, 0}, {437, 30, 1, 1, 0, 0, 0}, {440, 66, 35, 35, 0, 0, 0}, {9000, 113, 137, 137, 0, 0, 0}, {9005, 117, 34, 34, 0, 0, 0}, {10001, 209, 20, 20, 0, 0, 0}, {10002, 186, 41, 41, 0, 0, 0}, {10003, 4, 1, 1, 0, 0, 0}, {10004, 133, 9, 9, 0, 0, 0}, {10005, 103, 9, 9, 0, 0, 0}, {10006, 193, 4, 4, 0, 0, 0}, {10007, 71, 17, 17, 0, 0, 0}, {10008, 240, 14, 14, 0, 0, 0}, {10151, 195, 85, 85, 0, 0, 0}, {11000, 134, 51, 52, 3, 4, 5}, {11001, 15, 135, 136, 0, 0, 0}, {11002, 234, 179, 180, 3, 4, 5}, {11003, 64, 5, 5, 0, 0, 0}, {11004, 11, 232, 232, 3, 8, 9}, {11005, 93, 230, 230, 0, 0, 0}, {11010, 46, 49, 49, 0, 0, 0}, {11011, 106, 44, 44, 0, 0, 0}, {11020, 205, 16, 16, 0, 0, 0}, {11030, 144, 44, 44, 0, 0, 0}, {11031, 133, 44, 44, 0, 0, 0}, {11032, 85, 44, 44, 0, 0, 0}, {11033, 195, 37, 37, 3, 16, 17}, {11034, 79, 5, 5, 0, 0, 0}, {11035, 128, 8, 8, 3, 4, 5}, {11036, 177, 34, 34, 0, 0, 0}, {11037, 130, 28, 28, 0, 0, 0}, {11038, 47, 38, 38, 0, 0, 0}, {11039, 142, 9, 9, 0, 0, 0}, {11040, 132, 44, 44, 0, 0, 0}, {11041, 208, 44, 44, 0, 0, 0}, {11042, 201, 44, 44, 0, 0, 0}, {11043, 193, 44, 44, 0, 0, 0}, {11044, 189, 44, 44, 0, 0, 0}, {11060, 162, 78, 78, 0, 0, 0}, {12900, 114, 44, 44, 3, 0, 1}, {12901, 254, 59, 59, 3, 30, 31}, {12902, 140, 53, 53, 3, 4, 5}, {12903, 249, 46, 46, 3, 0, 1}, {12904, 77, 54, 54, 3, 28, 29}, {12905, 49, 43, 43, 3, 0, 1}, {12915, 94, 249, 249, 3, 0, 1}, {12918, 139, 51, 51, 0, 0, 0}, {12919, 7, 18, 18, 3, 16, 17}, {12920, 20, 5, 5, 0, 0, 0}, {42000, 227, 1, 1, 0, 0, 0}, {42001, 239, 46, 46, 0, 0, 0}, {50001, 246, 32, 32, 0, 0, 0}, {50002, 181, 246, 246, 0, 0, 0}, {50003, 62, 19, 19, 0, 0, 0}, {50004, 240, 10, 10, 3, 8, 9}, {50005, 152, 6, 6, 3, 4, 5}, {52000, 13, 100, 100, 0, 0, 0}, {52001, 239, 1, 1, 0, 0, 0}}
#endif

#include "../protocol.h"

#define MAVLINK_ENABLED_ARDUPILOTMEGA

// ENUM DEFINITIONS


/** @brief  */
#ifndef HAVE_ENUM_ACCELCAL_VEHICLE_POS
#define HAVE_ENUM_ACCELCAL_VEHICLE_POS
typedef enum ACCELCAL_VEHICLE_POS
{
   ACCELCAL_VEHICLE_POS_LEVEL=1, /*  | */
   ACCELCAL_VEHICLE_POS_LEFT=2, /*  | */
   ACCELCAL_VEHICLE_POS_RIGHT=3, /*  | */
   ACCELCAL_VEHICLE_POS_NOSEDOWN=4, /*  | */
   ACCELCAL_VEHICLE_POS_NOSEUP=5, /*  | */
   ACCELCAL_VEHICLE_POS_BACK=6, /*  | */
   ACCELCAL_VEHICLE_POS_SUCCESS=16777215, /*  | */
   ACCELCAL_VEHICLE_POS_FAILED=16777216, /*  | */
   ACCELCAL_VEHICLE_POS_ENUM_END=16777217, /*  | */
} ACCELCAL_VEHICLE_POS;
#endif

/** @brief  */
#ifndef HAVE_ENUM_HEADING_TYPE
#define HAVE_ENUM_HEADING_TYPE
typedef enum HEADING_TYPE
{
   HEADING_TYPE_COURSE_OVER_GROUND=0, /*  | */
   HEADING_TYPE_HEADING=1, /*  | */
   HEADING_TYPE_DEFAULT=2, /*  | */
   HEADING_TYPE_ENUM_END=3, /*  | */
} HEADING_TYPE;
#endif

/** @brief  */
#ifndef HAVE_ENUM_SCRIPTING_CMD
#define HAVE_ENUM_SCRIPTING_CMD
typedef enum SCRIPTING_CMD
{
   SCRIPTING_CMD_REPL_START=0, /* Start a REPL session. | */
   SCRIPTING_CMD_REPL_STOP=1, /* End a REPL session. | */
   SCRIPTING_CMD_STOP=2, /* Stop execution of scripts. | */
   SCRIPTING_CMD_STOP_AND_RESTART=3, /* Stop execution of scripts and restart. | */
   SCRIPTING_CMD_ENUM_END=4, /*  | */
} SCRIPTING_CMD;
#endif

/** @brief  */
#ifndef HAVE_ENUM_SECURE_COMMAND_OP
#define HAVE_ENUM_SECURE_COMMAND_OP
typedef enum SECURE_COMMAND_OP
{
   SECURE_COMMAND_GET_SESSION_KEY=0, /* Get an 8 byte session key which is used for remote secure updates which operate on flight controller data such as bootloader public keys. Return data will be 8 bytes on success. The session key remains valid until either the flight controller reboots or another SECURE_COMMAND_GET_SESSION_KEY is run. | */
   SECURE_COMMAND_GET_REMOTEID_SESSION_KEY=1, /* Get an 8 byte session key which is used for remote secure updates which operate on RemoteID module data. Return data will be 8 bytes on success. The session key remains valid until either the remote ID module reboots or another SECURE_COMMAND_GET_REMOTEID_SESSION_KEY is run. | */
   SECURE_COMMAND_REMOVE_PUBLIC_KEYS=2, /* Remove range of public keys from the bootloader. Command data consists of two bytes, first byte if index of first public key to remove. Second byte is the number of keys to remove. If all keys are removed then secure boot is disabled and insecure firmware can be loaded. | */
   SECURE_COMMAND_GET_PUBLIC_KEYS=3, /* Get current public keys from the bootloader. Command data consists of two bytes, first byte is index of first public key to fetch, 2nd byte is number of keys to fetch. Total data needs to fit in data portion of reply (max 6 keys for 32 byte keys). Reply data has the index of the first key in the first byte, followed by the keys. Returned keys may be less than the number of keys requested if there are less keys installed than requested. | */
   SECURE_COMMAND_SET_PUBLIC_KEYS=4, /* Set current public keys in the bootloader. Data consists of a one byte public key index followed by the public keys. With 32 byte keys this allows for up to 6 keys to be set in one request. Keys outside of the range that is being set will remain unchanged. | */
   SECURE_COMMAND_GET_REMOTEID_CONFIG=5, /* Get config data for remote ID module. This command should be sent to the component ID of the flight controller which will forward it to the RemoteID module either over mavlink or DroneCAN. Data format is specific to the RemoteID implementation, see RemoteID firmware documentation for details. | */
   SECURE_COMMAND_SET_REMOTEID_CONFIG=6, /* Set config data for remote ID module. This command should be sent to the component ID of the flight controller which will forward it to the RemoteID module either over mavlink or DroneCAN. Data format is specific to the RemoteID implementation, see RemoteID firmware documentation for details. | */
   SECURE_COMMAND_FLASH_BOOTLOADER=7, /* Flash bootloader from local storage. Data is the filename to use for the bootloader. This is intended to be used with MAVFtp to upload a new bootloader to a microSD before flashing. | */
   SECURE_COMMAND_OP_ENUM_END=8, /*  | */
} SECURE_COMMAND_OP;
#endif

/** @brief  */
#ifndef HAVE_ENUM_LIMITS_STATE
#define HAVE_ENUM_LIMITS_STATE
typedef enum LIMITS_STATE
{
   LIMITS_INIT=0, /* Pre-initialization. | */
   LIMITS_DISABLED=1, /* Disabled. | */
   LIMITS_ENABLED=2, /* Checking limits. | */
   LIMITS_TRIGGERED=3, /* A limit has been breached. | */
   LIMITS_RECOVERING=4, /* Taking action e.g. Return/RTL. | */
   LIMITS_RECOVERED=5, /* We're no longer in breach of a limit. | */
   LIMITS_STATE_ENUM_END=6, /*  | */
} LIMITS_STATE;
#endif

/** @brief  */
#ifndef HAVE_ENUM_LIMIT_MODULE
#define HAVE_ENUM_LIMIT_MODULE
typedef enum LIMIT_MODULE
{
   LIMIT_GPSLOCK=1, /* Pre-initialization. | */
   LIMIT_GEOFENCE=2, /* Disabled. | */
   LIMIT_ALTITUDE=4, /* Checking limits. | */
   LIMIT_MODULE_ENUM_END=5, /*  | */
} LIMIT_MODULE;
#endif

/** @brief Flags in RALLY_POINT message. */
#ifndef HAVE_ENUM_RALLY_FLAGS
#define HAVE_ENUM_RALLY_FLAGS
typedef enum RALLY_FLAGS
{
   FAVORABLE_WIND=1, /* Flag set when requiring favorable winds for landing. | */
   LAND_IMMEDIATELY=2, /* Flag set when plane is to immediately descend to break altitude and land without GCS intervention. Flag not set when plane is to loiter at Rally point until commanded to land. | */
   ALT_FRAME_VALID=4, /* True if the following altitude frame value is valid. | */
   ALT_FRAME=24, /* 2 bit value representing altitude frame. 0: absolute, 1: relative home, 2: relative origin, 3: relative terrain | */
   RALLY_FLAGS_ENUM_END=25, /*  | */
} RALLY_FLAGS;
#endif

/** @brief  */
#ifndef HAVE_ENUM_CAMERA_STATUS_TYPES
#define HAVE_ENUM_CAMERA_STATUS_TYPES
typedef enum CAMERA_STATUS_TYPES
{
   CAMERA_STATUS_TYPE_HEARTBEAT=0, /* Camera heartbeat, announce camera component ID at 1Hz. | */
   CAMERA_STATUS_TYPE_TRIGGER=1, /* Camera image triggered. | */
   CAMERA_STATUS_TYPE_DISCONNECT=2, /* Camera connection lost. | */
   CAMERA_STATUS_TYPE_ERROR=3, /* Camera unknown error. | */
   CAMERA_STATUS_TYPE_LOWBATT=4, /* Camera battery low. Parameter p1 shows reported voltage. | */
   CAMERA_STATUS_TYPE_LOWSTORE=5, /* Camera storage low. Parameter p1 shows reported shots remaining. | */
   CAMERA_STATUS_TYPE_LOWSTOREV=6, /* Camera storage low. Parameter p1 shows reported video minutes remaining. | */
   CAMERA_STATUS_TYPES_ENUM_END=7, /*  | */
} CAMERA_STATUS_TYPES;
#endif

/** @brief  */
#ifndef HAVE_ENUM_CAMERA_FEEDBACK_FLAGS
#define HAVE_ENUM_CAMERA_FEEDBACK_FLAGS
typedef enum CAMERA_FEEDBACK_FLAGS
{
   CAMERA_FEEDBACK_PHOTO=0, /* Shooting photos, not video. | */
   CAMERA_FEEDBACK_VIDEO=1, /* Shooting video, not stills. | */
   CAMERA_FEEDBACK_BADEXPOSURE=2, /* Unable to achieve requested exposure (e.g. shutter speed too low). | */
   CAMERA_FEEDBACK_CLOSEDLOOP=3, /* Closed loop feedback from camera, we know for sure it has successfully taken a picture. | */
   CAMERA_FEEDBACK_OPENLOOP=4, /* Open loop camera, an image trigger has been requested but we can't know for sure it has successfully taken a picture. | */
   CAMERA_FEEDBACK_FLAGS_ENUM_END=5, /*  | */
} CAMERA_FEEDBACK_FLAGS;
#endif

/** @brief  */
#ifndef HAVE_ENUM_MAV_MODE_GIMBAL
#define HAVE_ENUM_MAV_MODE_GIMBAL
typedef enum MAV_MODE_GIMBAL
{
   MAV_MODE_GIMBAL_UNINITIALIZED=0, /* Gimbal is powered on but has not started initializing yet. | */
   MAV_MODE_GIMBAL_CALIBRATING_PITCH=1, /* Gimbal is currently running calibration on the pitch axis. | */
   MAV_MODE_GIMBAL_CALIBRATING_ROLL=2, /* Gimbal is currently running calibration on the roll axis. | */
   MAV_MODE_GIMBAL_CALIBRATING_YAW=3, /* Gimbal is currently running calibration on the yaw axis. | */
   MAV_MODE_GIMBAL_INITIALIZED=4, /* Gimbal has finished calibrating and initializing, but is relaxed pending reception of first rate command from copter. | */
   MAV_MODE_GIMBAL_ACTIVE=5, /* Gimbal is actively stabilizing. | */
   MAV_MODE_GIMBAL_RATE_CMD_TIMEOUT=6, /* Gimbal is relaxed because it missed more than 10 expected rate command messages in a row. Gimbal will move back to active mode when it receives a new rate command. | */
   MAV_MODE_GIMBAL_ENUM_END=7, /*  | */
} MAV_MODE_GIMBAL;
#endif

/** @brief  */
#ifndef HAVE_ENUM_GIMBAL_AXIS
#define HAVE_ENUM_GIMBAL_AXIS
typedef enum GIMBAL_AXIS
{
   GIMBAL_AXIS_YAW=0, /* Gimbal yaw axis. | */
   GIMBAL_AXIS_PITCH=1, /* Gimbal pitch axis. | */
   GIMBAL_AXIS_ROLL=2, /* Gimbal roll axis. | */
   GIMBAL_AXIS_ENUM_END=3, /*  | */
} GIMBAL_AXIS;
#endif

/** @brief  */
#ifndef HAVE_ENUM_GIMBAL_AXIS_CALIBRATION_STATUS
#define HAVE_ENUM_GIMBAL_AXIS_CALIBRATION_STATUS
typedef enum GIMBAL_AXIS_CALIBRATION_STATUS
{
   GIMBAL_AXIS_CALIBRATION_STATUS_IN_PROGRESS=0, /* Axis calibration is in progress. | */
   GIMBAL_AXIS_CALIBRATION_STATUS_SUCCEEDED=1, /* Axis calibration succeeded. | */
   GIMBAL_AXIS_CALIBRATION_STATUS_FAILED=2, /* Axis calibration failed. | */
   GIMBAL_AXIS_CALIBRATION_STATUS_ENUM_END=3, /*  | */
} GIMBAL_AXIS_CALIBRATION_STATUS;
#endif

/** @brief  */
#ifndef HAVE_ENUM_GIMBAL_AXIS_CALIBRATION_REQUIRED
#define HAVE_ENUM_GIMBAL_AXIS_CALIBRATION_REQUIRED
typedef enum GIMBAL_AXIS_CALIBRATION_REQUIRED
{
   GIMBAL_AXIS_CALIBRATION_REQUIRED_UNKNOWN=0, /* Whether or not this axis requires calibration is unknown at this time. | */
   GIMBAL_AXIS_CALIBRATION_REQUIRED_TRUE=1, /* This axis requires calibration. | */
   GIMBAL_AXIS_CALIBRATION_REQUIRED_FALSE=2, /* This axis does not require calibration. | */
   GIMBAL_AXIS_CALIBRATION_REQUIRED_ENUM_END=3, /*  | */
} GIMBAL_AXIS_CALIBRATION_REQUIRED;
#endif

/** @brief  */
#ifndef HAVE_ENUM_GOPRO_HEARTBEAT_STATUS
#define HAVE_ENUM_GOPRO_HEARTBEAT_STATUS
typedef enum GOPRO_HEARTBEAT_STATUS
{
   GOPRO_HEARTBEAT_STATUS_DISCONNECTED=0, /* No GoPro connected. | */
   GOPRO_HEARTBEAT_STATUS_INCOMPATIBLE=1, /* The detected GoPro is not HeroBus compatible. | */
   GOPRO_HEARTBEAT_STATUS_CONNECTED=2, /* A HeroBus compatible GoPro is connected. | */
   GOPRO_HEARTBEAT_STATUS_ERROR=3, /* An unrecoverable error was encountered with the connected GoPro, it may require a power cycle. | */
   GOPRO_HEARTBEAT_STATUS_ENUM_END=4, /*  | */
} GOPRO_HEARTBEAT_STATUS;
#endif

/** @brief  */
#ifndef HAVE_ENUM_GOPRO_HEARTBEAT_FLAGS
#define HAVE_ENUM_GOPRO_HEARTBEAT_FLAGS
typedef enum GOPRO_HEARTBEAT_FLAGS
{
   GOPRO_FLAG_RECORDING=1, /* GoPro is currently recording. | */
   GOPRO_HEARTBEAT_FLAGS_ENUM_END=2, /*  | */
} GOPRO_HEARTBEAT_FLAGS;
#endif

/** @brief  */
#ifndef HAVE_ENUM_GOPRO_REQUEST_STATUS
#define HAVE_ENUM_GOPRO_REQUEST_STATUS
typedef enum GOPRO_REQUEST_STATUS
{
   GOPRO_REQUEST_SUCCESS=0, /* The write message with ID indicated succeeded. | */
   GOPRO_REQUEST_FAILED=1, /* The write message with ID indicated failed. | */
   GOPRO_REQUEST_STATUS_ENUM_END=2, /*  | */
} GOPRO_REQUEST_STATUS;
#endif

/** @brief  */
#ifndef HAVE_ENUM_GOPRO_COMMAND
#define HAVE_ENUM_GOPRO_COMMAND
typedef enum GOPRO_COMMAND
{
   GOPRO_COMMAND_POWER=0, /* (Get/Set). | */
   GOPRO_COMMAND_CAPTURE_MODE=1, /* (Get/Set). | */
   GOPRO_COMMAND_SHUTTER=2, /* (___/Set). | */
   GOPRO_COMMAND_BATTERY=3, /* (Get/___). | */
   GOPRO_COMMAND_MODEL=4, /* (Get/___). | */
   GOPRO_COMMAND_VIDEO_SETTINGS=5, /* (Get/Set). | */
   GOPRO_COMMAND_LOW_LIGHT=6, /* (Get/Set). | */
   GOPRO_COMMAND_PHOTO_RESOLUTION=7, /* (Get/Set). | */
   GOPRO_COMMAND_PHOTO_BURST_RATE=8, /* (Get/Set). | */
   GOPRO_COMMAND_PROTUNE=9, /* (Get/Set). | */
   GOPRO_COMMAND_PROTUNE_WHITE_BALANCE=10, /* (Get/Set) Hero 3+ Only. | */
   GOPRO_COMMAND_PROTUNE_COLOUR=11, /* (Get/Set) Hero 3+ Only. | */
   GOPRO_COMMAND_PROTUNE_GAIN=12, /* (Get/Set) Hero 3+ Only. | */
   GOPRO_COMMAND_PROTUNE_SHARPNESS=13, /* (Get/Set) Hero 3+ Only. | */
   GOPRO_COMMAND_PROTUNE_EXPOSURE=14, /* (Get/Set) Hero 3+ Only. | */
   GOPRO_COMMAND_TIME=15, /* (Get/Set). | */
   GOPRO_COMMAND_CHARGING=16, /* (Get/Set). | */
   GOPRO_COMMAND_ENUM_END=17, /*  | */
} GOPRO_COMMAND;
#endif

/** @brief  */
#ifndef HAVE_ENUM_GOPRO_CAPTURE_MODE
#define HAVE_ENUM_GOPRO_CAPTURE_MODE
typedef enum GOPRO_CAPTURE_MODE
{
   GOPRO_CAPTURE_MODE_VIDEO=0, /* Video mode. | */
   GOPRO_CAPTURE_MODE_PHOTO=1, /* Photo mode. | */
   GOPRO_CAPTURE_MODE_BURST=2, /* Burst mode, Hero 3+ only. | */
   GOPRO_CAPTURE_MODE_TIME_LAPSE=3, /* Time lapse mode, Hero 3+ only. | */
   GOPRO_CAPTURE_MODE_MULTI_SHOT=4, /* Multi shot mode, Hero 4 only. | */
   GOPRO_CAPTURE_MODE_PLAYBACK=5, /* Playback mode, Hero 4 only, silver only except when LCD or HDMI is connected to black. | */
   GOPRO_CAPTURE_MODE_SETUP=6, /* Playback mode, Hero 4 only. | */
   GOPRO_CAPTURE_MODE_UNKNOWN=255, /* Mode not yet known. | */
   GOPRO_CAPTURE_MODE_ENUM_END=256, /*  | */
} GOPRO_CAPTURE_MODE;
#endif

/** @brief  */
#ifndef HAVE_ENUM_GOPRO_RESOLUTION
#define HAVE_ENUM_GOPRO_RESOLUTION
typedef enum GOPRO_RESOLUTION
{
   GOPRO_RESOLUTION_480p=0, /* 848 x 480 (480p). | */
   GOPRO_RESOLUTION_720p=1, /* 1280 x 720 (720p). | */
   GOPRO_RESOLUTION_960p=2, /* 1280 x 960 (960p). | */
   GOPRO_RESOLUTION_1080p=3, /* 1920 x 1080 (1080p). | */
   GOPRO_RESOLUTION_1440p=4, /* 1920 x 1440 (1440p). | */
   GOPRO_RESOLUTION_2_7k_17_9=5, /* 2704 x 1440 (2.7k-17:9). | */
   GOPRO_RESOLUTION_2_7k_16_9=6, /* 2704 x 1524 (2.7k-16:9). | */
   GOPRO_RESOLUTION_2_7k_4_3=7, /* 2704 x 2028 (2.7k-4:3). | */
   GOPRO_RESOLUTION_4k_16_9=8, /* 3840 x 2160 (4k-16:9). | */
   GOPRO_RESOLUTION_4k_17_9=9, /* 4096 x 2160 (4k-17:9). | */
   GOPRO_RESOLUTION_720p_SUPERVIEW=10, /* 1280 x 720 (720p-SuperView). | */
   GOPRO_RESOLUTION_1080p_SUPERVIEW=11, /* 1920 x 1080 (1080p-SuperView). | */
   GOPRO_RESOLUTION_2_7k_SUPERVIEW=12, /* 2704 x 1520 (2.7k-SuperView). | */
   GOPRO_RESOLUTION_4k_SUPERVIEW=13, /* 3840 x 2160 (4k-SuperView). | */
   GOPRO_RESOLUTION_ENUM_END=14, /*  | */
} GOPRO_RESOLUTION;
#endif

/** @brief  */
#ifndef HAVE_ENUM_GOPRO_FRAME_RATE
#define HAVE_ENUM_GOPRO_FRAME_RATE
typedef enum GOPRO_FRAME_RATE
{
   GOPRO_FRAME_RATE_12=0, /* 12 FPS. | */
   GOPRO_FRAME_RATE_15=1, /* 15 FPS. | */
   GOPRO_FRAME_RATE_24=2, /* 24 FPS. | */
   GOPRO_FRAME_RATE_25=3, /* 25 FPS. | */
   GOPRO_FRAME_RATE_30=4, /* 30 FPS. | */
   GOPRO_FRAME_RATE_48=5, /* 48 FPS. | */
   GOPRO_FRAME_RATE_50=6, /* 50 FPS. | */
   GOPRO_FRAME_RATE_60=7, /* 60 FPS. | */
   GOPRO_FRAME_RATE_80=8, /* 80 FPS. | */
   GOPRO_FRAME_RATE_90=9, /* 90 FPS. | */
   GOPRO_FRAME_RATE_100=10, /* 100 FPS. | */
   GOPRO_FRAME_RATE_120=11, /* 120 FPS. | */
   GOPRO_FRAME_RATE_240=12, /* 240 FPS. | */
   GOPRO_FRAME_RATE_12_5=13, /* 12.5 FPS. | */
   GOPRO_FRAME_RATE_ENUM_END=14, /*  | */
} GOPRO_FRAME_RATE;
#endif

/** @brief  */
#ifndef HAVE_ENUM_GOPRO_FIELD_OF_VIEW
#define HAVE_ENUM_GOPRO_FIELD_OF_VIEW
typedef enum GOPRO_FIELD_OF_VIEW
{
   GOPRO_FIELD_OF_VIEW_WIDE=0, /* 0x00: Wide. | */
   GOPRO_FIELD_OF_VIEW_MEDIUM=1, /* 0x01: Medium. | */
   GOPRO_FIELD_OF_VIEW_NARROW=2, /* 0x02: Narrow. | */
   GOPRO_FIELD_OF_VIEW_ENUM_END=3, /*  | */
} GOPRO_FIELD_OF_VIEW;
#endif

/** @brief  */
#ifndef HAVE_ENUM_GOPRO_VIDEO_SETTINGS_FLAGS
#define HAVE_ENUM_GOPRO_VIDEO_SETTINGS_FLAGS
typedef enum GOPRO_VIDEO_SETTINGS_FLAGS
{
   GOPRO_VIDEO_SETTINGS_TV_MODE=1, /* 0=NTSC, 1=PAL. | */
   GOPRO_VIDEO_SETTINGS_FLAGS_ENUM_END=2, /*  | */
} GOPRO_VIDEO_SETTINGS_FLAGS;
#endif

/** @brief  */
#ifndef HAVE_ENUM_GOPRO_PHOTO_RESOLUTION
#define HAVE_ENUM_GOPRO_PHOTO_RESOLUTION
typedef enum GOPRO_PHOTO_RESOLUTION
{
   GOPRO_PHOTO_RESOLUTION_5MP_MEDIUM=0, /* 5MP Medium. | */
   GOPRO_PHOTO_RESOLUTION_7MP_MEDIUM=1, /* 7MP Medium. | */
   GOPRO_PHOTO_RESOLUTION_7MP_WIDE=2, /* 7MP Wide. | */
   GOPRO_PHOTO_RESOLUTION_10MP_WIDE=3, /* 10MP Wide. | */
   GOPRO_PHOTO_RESOLUTION_12MP_WIDE=4, /* 12MP Wide. | */
   GOPRO_PHOTO_RESOLUTION_ENUM_END=5, /*  | */
} GOPRO_PHOTO_RESOLUTION;
#endif

/** @brief  */
#ifndef HAVE_ENUM_GOPRO_PROTUNE_WHITE_BALANCE
#define HAVE_ENUM_GOPRO_PROTUNE_WHITE_BALANCE
typedef enum GOPRO_PROTUNE_WHITE_BALANCE
{
   GOPRO_PROTUNE_WHITE_BALANCE_AUTO=0, /* Auto. | */
   GOPRO_PROTUNE_WHITE_BALANCE_3000K=1, /* 3000K. | */
   GOPRO_PROTUNE_WHITE_BALANCE_5500K=2, /* 5500K. | */
   GOPRO_PROTUNE_WHITE_BALANCE_6500K=3, /* 6500K. | */
   GOPRO_PROTUNE_WHITE_BALANCE_RAW=4, /* Camera Raw. | */
   GOPRO_PROTUNE_WHITE_BALANCE_ENUM_END=5, /*  | */
} GOPRO_PROTUNE_WHITE_BALANCE;
#endif

/** @brief  */
#ifndef HAVE_ENUM_GOPRO_PROTUNE_COLOUR
#define HAVE_ENUM_GOPRO_PROTUNE_COLOUR
typedef enum GOPRO_PROTUNE_COLOUR
{
   GOPRO_PROTUNE_COLOUR_STANDARD=0, /* Auto. | */
   GOPRO_PROTUNE_COLOUR_NEUTRAL=1, /* Neutral. | */
   GOPRO_PROTUNE_COLOUR_ENUM_END=2, /*  | */
} GOPRO_PROTUNE_COLOUR;
#endif

/** @brief  */
#ifndef HAVE_ENUM_GOPRO_PROTUNE_GAIN
#define HAVE_ENUM_GOPRO_PROTUNE_GAIN
typedef enum GOPRO_PROTUNE_GAIN
{
   GOPRO_PROTUNE_GAIN_400=0, /* ISO 400. | */
   GOPRO_PROTUNE_GAIN_800=1, /* ISO 800 (Only Hero 4). | */
   GOPRO_PROTUNE_GAIN_1600=2, /* ISO 1600. | */
   GOPRO_PROTUNE_GAIN_3200=3, /* ISO 3200 (Only Hero 4). | */
   GOPRO_PROTUNE_GAIN_6400=4, /* ISO 6400. | */
   GOPRO_PROTUNE_GAIN_ENUM_END=5, /*  | */
} GOPRO_PROTUNE_GAIN;
#endif

/** @brief  */
#ifndef HAVE_ENUM_GOPRO_PROTUNE_SHARPNESS
#define HAVE_ENUM_GOPRO_PROTUNE_SHARPNESS
typedef enum GOPRO_PROTUNE_SHARPNESS
{
   GOPRO_PROTUNE_SHARPNESS_LOW=0, /* Low Sharpness. | */
   GOPRO_PROTUNE_SHARPNESS_MEDIUM=1, /* Medium Sharpness. | */
   GOPRO_PROTUNE_SHARPNESS_HIGH=2, /* High Sharpness. | */
   GOPRO_PROTUNE_SHARPNESS_ENUM_END=3, /*  | */
} GOPRO_PROTUNE_SHARPNESS;
#endif

/** @brief  */
#ifndef HAVE_ENUM_GOPRO_PROTUNE_EXPOSURE
#define HAVE_ENUM_GOPRO_PROTUNE_EXPOSURE
typedef enum GOPRO_PROTUNE_EXPOSURE
{
   GOPRO_PROTUNE_EXPOSURE_NEG_5_0=0, /* -5.0 EV (Hero 3+ Only). | */
   GOPRO_PROTUNE_EXPOSURE_NEG_4_5=1, /* -4.5 EV (Hero 3+ Only). | */
   GOPRO_PROTUNE_EXPOSURE_NEG_4_0=2, /* -4.0 EV (Hero 3+ Only). | */
   GOPRO_PROTUNE_EXPOSURE_NEG_3_5=3, /* -3.5 EV (Hero 3+ Only). | */
   GOPRO_PROTUNE_EXPOSURE_NEG_3_0=4, /* -3.0 EV (Hero 3+ Only). | */
   GOPRO_PROTUNE_EXPOSURE_NEG_2_5=5, /* -2.5 EV (Hero 3+ Only). | */
   GOPRO_PROTUNE_EXPOSURE_NEG_2_0=6, /* -2.0 EV. | */
   GOPRO_PROTUNE_EXPOSURE_NEG_1_5=7, /* -1.5 EV. | */
   GOPRO_PROTUNE_EXPOSURE_NEG_1_0=8, /* -1.0 EV. | */
   GOPRO_PROTUNE_EXPOSURE_NEG_0_5=9, /* -0.5 EV. | */
   GOPRO_PROTUNE_EXPOSURE_ZERO=10, /* 0.0 EV. | */
   GOPRO_PROTUNE_EXPOSURE_POS_0_5=11, /* +0.5 EV. | */
   GOPRO_PROTUNE_EXPOSURE_POS_1_0=12, /* +1.0 EV. | */
   GOPRO_PROTUNE_EXPOSURE_POS_1_5=13, /* +1.5 EV. | */
   GOPRO_PROTUNE_EXPOSURE_POS_2_0=14, /* +2.0 EV. | */
   GOPRO_PROTUNE_EXPOSURE_POS_2_5=15, /* +2.5 EV (Hero 3+ Only). | */
   GOPRO_PROTUNE_EXPOSURE_POS_3_0=16, /* +3.0 EV (Hero 3+ Only). | */
   GOPRO_PROTUNE_EXPOSURE_POS_3_5=17, /* +3.5 EV (Hero 3+ Only). | */
   GOPRO_PROTUNE_EXPOSURE_POS_4_0=18, /* +4.0 EV (Hero 3+ Only). | */
   GOPRO_PROTUNE_EXPOSURE_POS_4_5=19, /* +4.5 EV (Hero 3+ Only). | */
   GOPRO_PROTUNE_EXPOSURE_POS_5_0=20, /* +5.0 EV (Hero 3+ Only). | */
   GOPRO_PROTUNE_EXPOSURE_ENUM_END=21, /*  | */
} GOPRO_PROTUNE_EXPOSURE;
#endif

/** @brief  */
#ifndef HAVE_ENUM_GOPRO_CHARGING
#define HAVE_ENUM_GOPRO_CHARGING
typedef enum GOPRO_CHARGING
{
   GOPRO_CHARGING_DISABLED=0, /* Charging disabled. | */
   GOPRO_CHARGING_ENABLED=1, /* Charging enabled. | */
   GOPRO_CHARGING_ENUM_END=2, /*  | */
} GOPRO_CHARGING;
#endif

/** @brief  */
#ifndef HAVE_ENUM_GOPRO_MODEL
#define HAVE_ENUM_GOPRO_MODEL
typedef enum GOPRO_MODEL
{
   GOPRO_MODEL_UNKNOWN=0, /* Unknown gopro model. | */
   GOPRO_MODEL_HERO_3_PLUS_SILVER=1, /* Hero 3+ Silver (HeroBus not supported by GoPro). | */
   GOPRO_MODEL_HERO_3_PLUS_BLACK=2, /* Hero 3+ Black. | */
   GOPRO_MODEL_HERO_4_SILVER=3, /* Hero 4 Silver. | */
   GOPRO_MODEL_HERO_4_BLACK=4, /* Hero 4 Black. | */
   GOPRO_MODEL_ENUM_END=5, /*  | */
} GOPRO_MODEL;
#endif

/** @brief  */
#ifndef HAVE_ENUM_GOPRO_BURST_RATE
#define HAVE_ENUM_GOPRO_BURST_RATE
typedef enum GOPRO_BURST_RATE
{
   GOPRO_BURST_RATE_3_IN_1_SECOND=0, /* 3 Shots / 1 Second. | */
   GOPRO_BURST_RATE_5_IN_1_SECOND=1, /* 5 Shots / 1 Second. | */
   GOPRO_BURST_RATE_10_IN_1_SECOND=2, /* 10 Shots / 1 Second. | */
   GOPRO_BURST_RATE_10_IN_2_SECOND=3, /* 10 Shots / 2 Second. | */
   GOPRO_BURST_RATE_10_IN_3_SECOND=4, /* 10 Shots / 3 Second (Hero 4 Only). | */
   GOPRO_BURST_RATE_30_IN_1_SECOND=5, /* 30 Shots / 1 Second. | */
   GOPRO_BURST_RATE_30_IN_2_SECOND=6, /* 30 Shots / 2 Second. | */
   GOPRO_BURST_RATE_30_IN_3_SECOND=7, /* 30 Shots / 3 Second. | */
   GOPRO_BURST_RATE_30_IN_6_SECOND=8, /* 30 Shots / 6 Second. | */
   GOPRO_BURST_RATE_ENUM_END=9, /*  | */
} GOPRO_BURST_RATE;
#endif

/** @brief  */
#ifndef HAVE_ENUM_MAV_CMD_DO_AUX_FUNCTION_SWITCH_LEVEL
#define HAVE_ENUM_MAV_CMD_DO_AUX_FUNCTION_SWITCH_LEVEL
typedef enum MAV_CMD_DO_AUX_FUNCTION_SWITCH_LEVEL
{
   MAV_CMD_DO_AUX_FUNCTION_SWITCH_LEVEL_LOW=0, /* Switch Low. |Reserved (default:0)| Reserved (default:0)| Reserved (default:0)| Reserved (default:0)| Reserved (default:0)| Reserved (default:0)| Reserved (default:0)|  */
   MAV_CMD_DO_AUX_FUNCTION_SWITCH_LEVEL_MIDDLE=1, /* Switch Middle. |Reserved (default:0)| Reserved (default:0)| Reserved (default:0)| Reserved (default:0)| Reserved (default:0)| Reserved (default:0)| Reserved (default:0)|  */
   MAV_CMD_DO_AUX_FUNCTION_SWITCH_LEVEL_HIGH=2, /* Switch High. |Reserved (default:0)| Reserved (default:0)| Reserved (default:0)| Reserved (default:0)| Reserved (default:0)| Reserved (default:0)| Reserved (default:0)|  */
   MAV_CMD_DO_AUX_FUNCTION_SWITCH_LEVEL_ENUM_END=3, /*  | */
} MAV_CMD_DO_AUX_FUNCTION_SWITCH_LEVEL;
#endif

/** @brief  */
#ifndef HAVE_ENUM_LED_CONTROL_PATTERN
#define HAVE_ENUM_LED_CONTROL_PATTERN
typedef enum LED_CONTROL_PATTERN
{
   LED_CONTROL_PATTERN_OFF=0, /* LED patterns off (return control to regular vehicle control). | */
   LED_CONTROL_PATTERN_FIRMWAREUPDATE=1, /* LEDs show pattern during firmware update. | */
   LED_CONTROL_PATTERN_CUSTOM=255, /* Custom Pattern using custom bytes fields. | */
   LED_CONTROL_PATTERN_ENUM_END=256, /*  | */
} LED_CONTROL_PATTERN;
#endif

/** @brief Flags in EKF_STATUS message. */
#ifndef HAVE_ENUM_EKF_STATUS_FLAGS
#define HAVE_ENUM_EKF_STATUS_FLAGS
typedef enum EKF_STATUS_FLAGS
{
   EKF_ATTITUDE=1, /* Set if EKF's attitude estimate is good. | */
   EKF_VELOCITY_HORIZ=2, /* Set if EKF's horizontal velocity estimate is good. | */
   EKF_VELOCITY_VERT=4, /* Set if EKF's vertical velocity estimate is good. | */
   EKF_POS_HORIZ_REL=8, /* Set if EKF's horizontal position (relative) estimate is good. | */
   EKF_POS_HORIZ_ABS=16, /* Set if EKF's horizontal position (absolute) estimate is good. | */
   EKF_POS_VERT_ABS=32, /* Set if EKF's vertical position (absolute) estimate is good. | */
   EKF_POS_VERT_AGL=64, /* Set if EKF's vertical position (above ground) estimate is good. | */
   EKF_CONST_POS_MODE=128, /* EKF is in constant position mode and does not know it's absolute or relative position. | */
   EKF_PRED_POS_HORIZ_REL=256, /* Set if EKF's predicted horizontal position (relative) estimate is good. | */
   EKF_PRED_POS_HORIZ_ABS=512, /* Set if EKF's predicted horizontal position (absolute) estimate is good. | */
   EKF_UNINITIALIZED=1024, /* Set if EKF has never been healthy. | */
   EKF_GPS_GLITCHING=32768, /* Set if EKF believes the GPS input data is faulty. | */
   EKF_STATUS_FLAGS_ENUM_END=32769, /*  | */
} EKF_STATUS_FLAGS;
#endif

/** @brief  */
#ifndef HAVE_ENUM_PID_TUNING_AXIS
#define HAVE_ENUM_PID_TUNING_AXIS
typedef enum PID_TUNING_AXIS
{
   PID_TUNING_ROLL=1, /*  | */
   PID_TUNING_PITCH=2, /*  | */
   PID_TUNING_YAW=3, /*  | */
   PID_TUNING_ACCZ=4, /*  | */
   PID_TUNING_STEER=5, /*  | */
   PID_TUNING_LANDING=6, /*  | */
   PID_TUNING_AXIS_ENUM_END=7, /*  | */
} PID_TUNING_AXIS;
#endif

/** @brief Special ACK block numbers control activation of dataflash log streaming. */
#ifndef HAVE_ENUM_MAV_REMOTE_LOG_DATA_BLOCK_COMMANDS
#define HAVE_ENUM_MAV_REMOTE_LOG_DATA_BLOCK_COMMANDS
typedef enum MAV_REMOTE_LOG_DATA_BLOCK_COMMANDS
{
   MAV_REMOTE_LOG_DATA_BLOCK_STOP=2147483645, /* UAV to stop sending DataFlash blocks. | */
   MAV_REMOTE_LOG_DATA_BLOCK_START=2147483646, /* UAV to start sending DataFlash blocks. | */
   MAV_REMOTE_LOG_DATA_BLOCK_COMMANDS_ENUM_END=2147483647, /*  | */
} MAV_REMOTE_LOG_DATA_BLOCK_COMMANDS;
#endif

/** @brief Possible remote log data block statuses. */
#ifndef HAVE_ENUM_MAV_REMOTE_LOG_DATA_BLOCK_STATUSES
#define HAVE_ENUM_MAV_REMOTE_LOG_DATA_BLOCK_STATUSES
typedef enum MAV_REMOTE_LOG_DATA_BLOCK_STATUSES
{
   MAV_REMOTE_LOG_DATA_BLOCK_NACK=0, /* This block has NOT been received. | */
   MAV_REMOTE_LOG_DATA_BLOCK_ACK=1, /* This block has been received. | */
   MAV_REMOTE_LOG_DATA_BLOCK_STATUSES_ENUM_END=2, /*  | */
} MAV_REMOTE_LOG_DATA_BLOCK_STATUSES;
#endif

/** @brief Bus types for device operations. */
#ifndef HAVE_ENUM_DEVICE_OP_BUSTYPE
#define HAVE_ENUM_DEVICE_OP_BUSTYPE
typedef enum DEVICE_OP_BUSTYPE
{
   DEVICE_OP_BUSTYPE_I2C=0, /* I2C Device operation. | */
   DEVICE_OP_BUSTYPE_SPI=1, /* SPI Device operation. | */
   DEVICE_OP_BUSTYPE_ENUM_END=2, /*  | */
} DEVICE_OP_BUSTYPE;
#endif

/** @brief Deepstall flight stage. */
#ifndef HAVE_ENUM_DEEPSTALL_STAGE
#define HAVE_ENUM_DEEPSTALL_STAGE
typedef enum DEEPSTALL_STAGE
{
   DEEPSTALL_STAGE_FLY_TO_LANDING=0, /* Flying to the landing point. | */
   DEEPSTALL_STAGE_ESTIMATE_WIND=1, /* Building an estimate of the wind. | */
   DEEPSTALL_STAGE_WAIT_FOR_BREAKOUT=2, /* Waiting to breakout of the loiter to fly the approach. | */
   DEEPSTALL_STAGE_FLY_TO_ARC=3, /* Flying to the first arc point to turn around to the landing point. | */
   DEEPSTALL_STAGE_ARC=4, /* Turning around back to the deepstall landing point. | */
   DEEPSTALL_STAGE_APPROACH=5, /* Approaching the landing point. | */
   DEEPSTALL_STAGE_LAND=6, /* Stalling and steering towards the land point. | */
   DEEPSTALL_STAGE_ENUM_END=7, /*  | */
} DEEPSTALL_STAGE;
#endif

/** @brief A mapping of plane flight modes for custom_mode field of heartbeat. */
#ifndef HAVE_ENUM_PLANE_MODE
#define HAVE_ENUM_PLANE_MODE
typedef enum PLANE_MODE
{
   PLANE_MODE_MANUAL=0, /* MANUAL | */
   PLANE_MODE_CIRCLE=1, /* CIRCLE | */
   PLANE_MODE_STABILIZE=2, /* STABILIZE | */
   PLANE_MODE_TRAINING=3, /* TRAINING | */
   PLANE_MODE_ACRO=4, /* ACRO | */
   PLANE_MODE_FLY_BY_WIRE_A=5, /* FBWA | */
   PLANE_MODE_FLY_BY_WIRE_B=6, /* FBWB | */
   PLANE_MODE_CRUISE=7, /* CRUISE | */
   PLANE_MODE_AUTOTUNE=8, /* AUTOTUNE | */
   PLANE_MODE_AUTO=10, /* AUTO | */
   PLANE_MODE_RTL=11, /* RTL | */
   PLANE_MODE_LOITER=12, /* LOITER | */
   PLANE_MODE_TAKEOFF=13, /* TAKEOFF | */
   PLANE_MODE_AVOID_ADSB=14, /* AVOID ADSB | */
   PLANE_MODE_GUIDED=15, /* GUIDED | */
   PLANE_MODE_INITIALIZING=16, /* INITIALISING | */
   PLANE_MODE_QSTABILIZE=17, /* QSTABILIZE | */
   PLANE_MODE_QHOVER=18, /* QHOVER | */
   PLANE_MODE_QLOITER=19, /* QLOITER | */
   PLANE_MODE_QLAND=20, /* QLAND | */
   PLANE_MODE_QRTL=21, /* QRTL | */
   PLANE_MODE_QAUTOTUNE=22, /* QAUTOTUNE | */
   PLANE_MODE_QACRO=23, /* QACRO | */
   PLANE_MODE_THERMAL=24, /* THERMAL | */
   PLANE_MODE_LOITER_ALT_QLAND=25, /* LOITER2QLAND | */
   PLANE_MODE_AUTOLAND=26, /* AUTOLAND | */
   PLANE_MODE_ENUM_END=27, /*  | */
} PLANE_MODE;
#endif

/** @brief A mapping of copter flight modes for custom_mode field of heartbeat. */
#ifndef HAVE_ENUM_COPTER_MODE
#define HAVE_ENUM_COPTER_MODE
typedef enum COPTER_MODE
{
   COPTER_MODE_STABILIZE=0, /* STABILIZE | */
   COPTER_MODE_ACRO=1, /* ACRO | */
   COPTER_MODE_ALT_HOLD=2, /* ALT HOLD | */
   COPTER_MODE_AUTO=3, /* AUTO | */
   COPTER_MODE_GUIDED=4, /* GUIDED | */
   COPTER_MODE_LOITER=5, /* LOITER | */
   COPTER_MODE_RTL=6, /* RTL | */
   COPTER_MODE_CIRCLE=7, /* CIRCLE | */
   COPTER_MODE_LAND=9, /* LAND | */
   COPTER_MODE_DRIFT=11, /* DRIFT | */
   COPTER_MODE_SPORT=13, /* SPORT | */
   COPTER_MODE_FLIP=14, /* FLIP | */
   COPTER_MODE_AUTOTUNE=15, /* AUTOTUNE | */
   COPTER_MODE_POSHOLD=16, /* POSHOLD | */
   COPTER_MODE_BRAKE=17, /* BRAKE | */
   COPTER_MODE_THROW=18, /* THROW | */
   COPTER_MODE_AVOID_ADSB=19, /* AVOID ADSB | */
   COPTER_MODE_GUIDED_NOGPS=20, /* GUIDED NOGPS | */
   COPTER_MODE_SMART_RTL=21, /* SMARTRTL | */
   COPTER_MODE_FLOWHOLD=22, /* FLOWHOLD | */
   COPTER_MODE_FOLLOW=23, /* FOLLOW | */
   COPTER_MODE_ZIGZAG=24, /* ZIGZAG | */
   COPTER_MODE_SYSTEMID=25, /* SYSTEMID | */
   COPTER_MODE_AUTOROTATE=26, /* AUTOROTATE | */
   COPTER_MODE_AUTO_RTL=27, /* AUTO RTL | */
   COPTER_MODE_TURTLE=28, /* TURTLE | */
   COPTER_MODE_RATE_ACRO=29, /* RATE_ACRO | */
   COPTER_MODE_ENUM_END=30, /*  | */
} COPTER_MODE;
#endif

/** @brief A mapping of sub flight modes for custom_mode field of heartbeat. */
#ifndef HAVE_ENUM_SUB_MODE
#define HAVE_ENUM_SUB_MODE
typedef enum SUB_MODE
{
   SUB_MODE_STABILIZE=0, /* STABILIZE | */
   SUB_MODE_ACRO=1, /* ACRO | */
   SUB_MODE_ALT_HOLD=2, /* ALT HOLD | */
   SUB_MODE_AUTO=3, /* AUTO | */
   SUB_MODE_GUIDED=4, /* GUIDED | */
   SUB_MODE_CIRCLE=7, /* CIRCLE | */
   SUB_MODE_SURFACE=9, /* SURFACE | */
   SUB_MODE_POSHOLD=16, /* POSHOLD | */
   SUB_MODE_MANUAL=19, /* MANUAL | */
   SUB_MODE_MOTORDETECT=20, /* MOTORDETECT | */
   SUB_MODE_SURFTRAK=21, /* SURFTRAK | */
   SUB_MODE_ENUM_END=22, /*  | */
} SUB_MODE;
#endif

/** @brief A mapping of rover flight modes for custom_mode field of heartbeat. */
#ifndef HAVE_ENUM_ROVER_MODE
#define HAVE_ENUM_ROVER_MODE
typedef enum ROVER_MODE
{
   ROVER_MODE_MANUAL=0, /* MANUAL | */
   ROVER_MODE_ACRO=1, /* ACRO | */
   ROVER_MODE_STEERING=3, /* STEERING | */
   ROVER_MODE_HOLD=4, /* HOLD | */
   ROVER_MODE_LOITER=5, /* LOITER | */
   ROVER_MODE_FOLLOW=6, /* FOLLOW | */
   ROVER_MODE_SIMPLE=7, /* SIMPLE | */
   ROVER_MODE_DOCK=8, /* DOCK | */
   ROVER_MODE_CIRCLE=9, /* CIRCLE | */
   ROVER_MODE_AUTO=10, /* AUTO | */
   ROVER_MODE_RTL=11, /* RTL | */
   ROVER_MODE_SMART_RTL=12, /* SMART RTL | */
   ROVER_MODE_GUIDED=15, /* GUIDED | */
   ROVER_MODE_INITIALIZING=16, /* INITIALISING | */
   ROVER_MODE_ENUM_END=17, /*  | */
} ROVER_MODE;
#endif

/** @brief A mapping of antenna tracker flight modes for custom_mode field of heartbeat. */
#ifndef HAVE_ENUM_TRACKER_MODE
#define HAVE_ENUM_TRACKER_MODE
typedef enum TRACKER_MODE
{
   TRACKER_MODE_MANUAL=0, /* MANUAL | */
   TRACKER_MODE_STOP=1, /* STOP | */
   TRACKER_MODE_SCAN=2, /* SCAN | */
   TRACKER_MODE_SERVO_TEST=3, /* SERVO TEST | */
   TRACKER_MODE_GUIDED=4, /* GUIDED | */
   TRACKER_MODE_AUTO=10, /* AUTO | */
   TRACKER_MODE_INITIALIZING=16, /* INITIALISING | */
   TRACKER_MODE_ENUM_END=17, /*  | */
} TRACKER_MODE;
#endif

/** @brief The type of parameter for the OSD parameter editor. */
#ifndef HAVE_ENUM_OSD_PARAM_CONFIG_TYPE
#define HAVE_ENUM_OSD_PARAM_CONFIG_TYPE
typedef enum OSD_PARAM_CONFIG_TYPE
{
   OSD_PARAM_NONE=0, /*  | */
   OSD_PARAM_SERIAL_PROTOCOL=1, /*  | */
   OSD_PARAM_SERVO_FUNCTION=2, /*  | */
   OSD_PARAM_AUX_FUNCTION=3, /*  | */
   OSD_PARAM_FLIGHT_MODE=4, /*  | */
   OSD_PARAM_FAILSAFE_ACTION=5, /*  | */
   OSD_PARAM_FAILSAFE_ACTION_1=6, /*  | */
   OSD_PARAM_FAILSAFE_ACTION_2=7, /*  | */
   OSD_PARAM_NUM_TYPES=8, /*  | */
   OSD_PARAM_CONFIG_TYPE_ENUM_END=9, /*  | */
} OSD_PARAM_CONFIG_TYPE;
#endif

/** @brief The error type for the OSD parameter editor. */
#ifndef HAVE_ENUM_OSD_PARAM_CONFIG_ERROR
#define HAVE_ENUM_OSD_PARAM_CONFIG_ERROR
typedef enum OSD_PARAM_CONFIG_ERROR
{
   OSD_PARAM_SUCCESS=0, /*  | */
   OSD_PARAM_INVALID_SCREEN=1, /*  | */
   OSD_PARAM_INVALID_PARAMETER_INDEX=2, /*  | */
   OSD_PARAM_INVALID_PARAMETER=3, /*  | */
   OSD_PARAM_CONFIG_ERROR_ENUM_END=4, /*  | */
} OSD_PARAM_CONFIG_ERROR;
#endif

// MAVLINK VERSION

#ifndef MAVLINK_VERSION
#define MAVLINK_VERSION 2
#endif

#if (MAVLINK_VERSION == 0)
#undef MAVLINK_VERSION
#define MAVLINK_VERSION 2
#endif

// MESSAGE DEFINITIONS
#include "./mavlink_msg_sensor_offsets.h"
#include "./mavlink_msg_set_mag_offsets.h"
#include "./mavlink_msg_meminfo.h"
#include "./mavlink_msg_ap_adc.h"
#include "./mavlink_msg_digicam_configure.h"
#include "./mavlink_msg_digicam_control.h"
#include "./mavlink_msg_mount_configure.h"
#include "./mavlink_msg_mount_control.h"
#include "./mavlink_msg_mount_status.h"
#include "./mavlink_msg_fence_point.h"
#include "./mavlink_msg_fence_fetch_point.h"
#include "./mavlink_msg_ahrs.h"
#include "./mavlink_msg_simstate.h"
#include "./mavlink_msg_hwstatus.h"
#include "./mavlink_msg_radio.h"
#include "./mavlink_msg_limits_status.h"
#include "./mavlink_msg_wind.h"
#include "./mavlink_msg_data16.h"
#include "./mavlink_msg_data32.h"
#include "./mavlink_msg_data64.h"
#include "./mavlink_msg_data96.h"
#include "./mavlink_msg_rangefinder.h"
#include "./mavlink_msg_airspeed_autocal.h"
#include "./mavlink_msg_rally_point.h"
#include "./mavlink_msg_rally_fetch_point.h"
#include "./mavlink_msg_compassmot_status.h"
#include "./mavlink_msg_ahrs2.h"
#include "./mavlink_msg_camera_status.h"
#include "./mavlink_msg_camera_feedback.h"
#include "./mavlink_msg_battery2.h"
#include "./mavlink_msg_ahrs3.h"
#include "./mavlink_msg_autopilot_version_request.h"
#include "./mavlink_msg_remote_log_data_block.h"
#include "./mavlink_msg_remote_log_block_status.h"
#include "./mavlink_msg_led_control.h"
#include "./mavlink_msg_mag_cal_progress.h"
#include "./mavlink_msg_ekf_status_report.h"
#include "./mavlink_msg_pid_tuning.h"
#include "./mavlink_msg_deepstall.h"
#include "./mavlink_msg_gimbal_report.h"
#include "./mavlink_msg_gimbal_control.h"
#include "./mavlink_msg_gimbal_torque_cmd_report.h"
#include "./mavlink_msg_gopro_heartbeat.h"
#include "./mavlink_msg_gopro_get_request.h"
#include "./mavlink_msg_gopro_get_response.h"
#include "./mavlink_msg_gopro_set_request.h"
#include "./mavlink_msg_gopro_set_response.h"
#include "./mavlink_msg_rpm.h"
#include "./mavlink_msg_device_op_read.h"
#include "./mavlink_msg_device_op_read_reply.h"
#include "./mavlink_msg_device_op_write.h"
#include "./mavlink_msg_device_op_write_reply.h"
#include "./mavlink_msg_secure_command.h"
#include "./mavlink_msg_secure_command_reply.h"
#include "./mavlink_msg_adap_tuning.h"
#include "./mavlink_msg_vision_position_delta.h"
#include "./mavlink_msg_aoa_ssa.h"
#include "./mavlink_msg_esc_telemetry_1_to_4.h"
#include "./mavlink_msg_esc_telemetry_5_to_8.h"
#include "./mavlink_msg_esc_telemetry_9_to_12.h"
#include "./mavlink_msg_osd_param_config.h"
#include "./mavlink_msg_osd_param_config_reply.h"
#include "./mavlink_msg_osd_param_show_config.h"
#include "./mavlink_msg_osd_param_show_config_reply.h"
#include "./mavlink_msg_obstacle_distance_3d.h"
#include "./mavlink_msg_water_depth.h"
#include "./mavlink_msg_mcu_status.h"
#include "./mavlink_msg_esc_telemetry_13_to_16.h"
#include "./mavlink_msg_esc_telemetry_17_to_20.h"
#include "./mavlink_msg_esc_telemetry_21_to_24.h"
#include "./mavlink_msg_esc_telemetry_25_to_28.h"
#include "./mavlink_msg_esc_telemetry_29_to_32.h"
#include "./mavlink_msg_named_value_string.h"

// base include
#include "../common/common.h"
#include "../uAvionix/uAvionix.h"
#include "../icarous/icarous.h"
#include "../loweheiser/loweheiser.h"
#include "../cubepilot/cubepilot.h"
#include "../csAirLink/csAirLink.h"


#if MAVLINK_ARDUPILOTMEGA_XML_HASH == MAVLINK_PRIMARY_XML_HASH
# define MAVLINK_MESSAGE_INFO {MAVLINK_MESSAGE_INFO_HEARTBEAT, MAVLINK_MESSAGE_INFO_SYS_STATUS, MAVLINK_MESSAGE_INFO_SYSTEM_TIME, MAVLINK_MESSAGE_INFO_PING, MAVLINK_MESSAGE_INFO_CHANGE_OPERATOR_CONTROL, MAVLINK_MESSAGE_INFO_CHANGE_OPERATOR_CONTROL_ACK, MAVLINK_MESSAGE_INFO_AUTH_KEY, MAVLINK_MESSAGE_INFO_LINK_NODE_STATUS, MAVLINK_MESSAGE_INFO_SET_MODE, MAVLINK_MESSAGE_INFO_PARAM_REQUEST_READ, MAVLINK_MESSAGE_INFO_PARAM_REQUEST_LIST, MAVLINK_MESSAGE_INFO_PARAM_VALUE, MAVLINK_MESSAGE_INFO_PARAM_SET, MAVLINK_MESSAGE_INFO_GPS_RAW_INT, MAVLINK_MESSAGE_INFO_GPS_STATUS, MAVLINK_MESSAGE_INFO_SCALED_IMU, MAVLINK_MESSAGE_INFO_RAW_IMU, MAVLINK_MESSAGE_INFO_RAW_PRESSURE, MAVLINK_MESSAGE_INFO_SCALED_PRESSURE, MAVLINK_MESSAGE_INFO_ATTITUDE, MAVLINK_MESSAGE_INFO_ATTITUDE_QUATERNION, MAVLINK_MESSAGE_INFO_LOCAL_POSITION_NED, MAVLINK_MESSAGE_INFO_GLOBAL_POSITION_INT, MAVLINK_MESSAGE_INFO_RC_CHANNELS_SCALED, MAVLINK_MESSAGE_INFO_RC_CHANNELS_RAW, MAVLINK_MESSAGE_INFO_SERVO_OUTPUT_RAW, MAVLINK_MESSAGE_INFO_MISSION_REQUEST_PARTIAL_LIST, MAVLINK_MESSAGE_INFO_MISSION_WRITE_PARTIAL_LIST, MAVLINK_MESSAGE_INFO_MISSION_ITEM, MAVLINK_MESSAGE_INFO_MISSION_REQUEST, MAVLINK_MESSAGE_INFO_MISSION_SET_CURRENT, MAVLINK_MESSAGE_INFO_MISSION_CURRENT, MAVLINK_MESSAGE_INFO_MISSION_REQUEST_LIST, MAVLINK_MESSAGE_INFO_MISSION_COUNT, MAVLINK_MESSAGE_INFO_MISSION_CLEAR_ALL, MAVLINK_MESSAGE_INFO_MISSION_ITEM_REACHED, MAVLINK_MESSAGE_INFO_MISSION_ACK, MAVLINK_MESSAGE_INFO_SET_GPS_GLOBAL_ORIGIN, MAVLINK_MESSAGE_INFO_GPS_GLOBAL_ORIGIN, MAVLINK_MESSAGE_INFO_PARAM_MAP_RC, MAVLINK_MESSAGE_INFO_MISSION_REQUEST_INT, MAVLINK_MESSAGE_INFO_SAFETY_SET_ALLOWED_AREA, MAVLINK_MESSAGE_INFO_SAFETY_ALLOWED_AREA, MAVLINK_MESSAGE_INFO_ATTITUDE_QUATERNION_COV, MAVLINK_MESSAGE_INFO_NAV_CONTROLLER_OUTPUT, MAVLINK_MESSAGE_INFO_GLOBAL_POSITION_INT_COV, MAVLINK_MESSAGE_INFO_LOCAL_POSITION_NED_COV, MAVLINK_MESSAGE_INFO_RC_CHANNELS, MAVLINK_MESSAGE_INFO_REQUEST_DATA_STREAM, MAVLINK_MESSAGE_INFO_DATA_STREAM, MAVLINK_MESSAGE_INFO_MANUAL_CONTROL, MAVLINK_MESSAGE_INFO_RC_CHANNELS_OVERRIDE, MAVLINK_MESSAGE_INFO_MISSION_ITEM_INT, MAVLINK_MESSAGE_INFO_VFR_HUD, MAVLINK_MESSAGE_INFO_COMMAND_INT, MAVLINK_MESSAGE_INFO_COMMAND_LONG, MAVLINK_MESSAGE_INFO_COMMAND_ACK, MAVLINK_MESSAGE_INFO_COMMAND_CANCEL, MAVLINK_MESSAGE_INFO_MANUAL_SETPOINT, MAVLINK_MESSAGE_INFO_SET_ATTITUDE_TARGET, MAVLINK_MESSAGE_INFO_ATTITUDE_TARGET, MAVLINK_MESSAGE_INFO_SET_POSITION_TARGET_LOCAL_NED, MAVLINK_MESSAGE_INFO_POSITION_TARGET_LOCAL_NED, MAVLINK_MESSAGE_INFO_SET_POSITION_TARGET_GLOBAL_INT, MAVLINK_MESSAGE_INFO_POSITION_TARGET_GLOBAL_INT, MAVLINK_MESSAGE_INFO_LOCAL_POSITION_NED_SYSTEM_GLOBAL_OFFSET, MAVLINK_MESSAGE_INFO_HIL_STATE, MAVLINK_MESSAGE_INFO_HIL_CONTROLS, MAVLINK_MESSAGE_INFO_HIL_RC_INPUTS_RAW, MAVLINK_MESSAGE_INFO_HIL_ACTUATOR_CONTROLS, MAVLINK_MESSAGE_INFO_OPTICAL_FLOW, MAVLINK_MESSAGE_INFO_GLOBAL_VISION_POSITION_ESTIMATE, MAVLINK_MESSAGE_INFO_VISION_POSITION_ESTIMATE, MAVLINK_MESSAGE_INFO_VISION_SPEED_ESTIMATE, MAVLINK_MESSAGE_INFO_VICON_POSITION_ESTIMATE, MAVLINK_MESSAGE_INFO_HIGHRES_IMU, MAVLINK_MESSAGE_INFO_OPTICAL_FLOW_RAD, MAVLINK_MESSAGE_INFO_HIL_SENSOR, MAVLINK_MESSAGE_INFO_SIM_STATE, MAVLINK_MESSAGE_INFO_RADIO_STATUS, MAVLINK_MESSAGE_INFO_FILE_TRANSFER_PROTOCOL, MAVLINK_MESSAGE_INFO_TIMESYNC, MAVLINK_MESSAGE_INFO_CAMERA_TRIGGER, MAVLINK_MESSAGE_INFO_HIL_GPS, MAVLINK_MESSAGE_INFO_HIL_OPTICAL_FLOW, MAVLINK_MESSAGE_INFO_HIL_STATE_QUATERNION, MAVLINK_MESSAGE_INFO_SCALED_IMU2, MAVLINK_MESSAGE_INFO_LOG_REQUEST_LIST, MAVLINK_MESSAGE_INFO_LOG_ENTRY, MAVLINK_MESSAGE_INFO_LOG_REQUEST_DATA, MAVLINK_MESSAGE_INFO_LOG_DATA, MAVLINK_MESSAGE_INFO_LOG_ERASE, MAVLINK_MESSAGE_INFO_LOG_REQUEST_END, MAVLINK_MESSAGE_INFO_GPS_INJECT_DATA, MAVLINK_MESSAGE_INFO_GPS2_RAW, MAVLINK_MESSAGE_INFO_POWER_STATUS, MAVLINK_MESSAGE_INFO_SERIAL_CONTROL, MAVLINK_MESSAGE_INFO_GPS_RTK, MAVLINK_MESSAGE_INFO_GPS2_RTK, MAVLINK_MESSAGE_INFO_SCALED_IMU3, MAVLINK_MESSAGE_INFO_DATA_TRANSMISSION_HANDSHAKE, MAVLINK_MESSAGE_INFO_ENCAPSULATED_DATA, MAVLINK_MESSAGE_INFO_DISTANCE_SENSOR, MAVLINK_MESSAGE_INFO_TERRAIN_REQUEST, MAVLINK_MESSAGE_INFO_TERRAIN_DATA, MAVLINK_MESSAGE_INFO_TERRAIN_CHECK, MAVLINK_MESSAGE_INFO_TERRAIN_REPORT, MAVLINK_MESSAGE_INFO_SCALED_PRESSURE2, MAVLINK_MESSAGE_INFO_ATT_POS_MOCAP, MAVLINK_MESSAGE_INFO_SET_ACTUATOR_CONTROL_TARGET, MAVLINK_MESSAGE_INFO_ACTUATOR_CONTROL_TARGET, MAVLINK_MESSAGE_INFO_ALTITUDE, MAVLINK_MESSAGE_INFO_RESOURCE_REQUEST, MAVLINK_MESSAGE_INFO_SCALED_PRESSURE3, MAVLINK_MESSAGE_INFO_FOLLOW_TARGET, MAVLINK_MESSAGE_INFO_CONTROL_SYSTEM_STATE, MAVLINK_MESSAGE_INFO_BATTERY_STATUS, MAVLINK_MESSAGE_INFO_AUTOPILOT_VERSION, MAVLINK_MESSAGE_INFO_LANDING_TARGET, MAVLINK_MESSAGE_INFO_SENSOR_OFFSETS, MAVLINK_MESSAGE_INFO_SET_MAG_OFFSETS, MAVLINK_MESSAGE_INFO_MEMINFO, MAVLINK_MESSAGE_INFO_AP_ADC, MAVLINK_MESSAGE_INFO_DIGICAM_CONFIGURE, MAVLINK_MESSAGE_INFO_DIGICAM_CONTROL, MAVLINK_MESSAGE_INFO_MOUNT_CONFIGURE, MAVLINK_MESSAGE_INFO_MOUNT_CONTROL, MAVLINK_MESSAGE_INFO_MOUNT_STATUS, MAVLINK_MESSAGE_INFO_FENCE_POINT, MAVLINK_MESSAGE_INFO_FENCE_FETCH_POINT, MAVLINK_MESSAGE_INFO_FENCE_STATUS, MAVLINK_MESSAGE_INFO_AHRS, MAVLINK_MESSAGE_INFO_SIMSTATE, MAVLINK_MESSAGE_INFO_HWSTATUS, MAVLINK_MESSAGE_INFO_RADIO, MAVLINK_MESSAGE_INFO_LIMITS_STATUS, MAVLINK_MESSAGE_INFO_WIND, MAVLINK_MESSAGE_INFO_DATA16, MAVLINK_MESSAGE_INFO_DATA32, MAVLINK_MESSAGE_INFO_DATA64, MAVLINK_MESSAGE_INFO_DATA96, MAVLINK_MESSAGE_INFO_RANGEFINDER, MAVLINK_MESSAGE_INFO_AIRSPEED_AUTOCAL, MAVLINK_MESSAGE_INFO_RALLY_POINT, MAVLINK_MESSAGE_INFO_RALLY_FETCH_POINT, MAVLINK_MESSAGE_INFO_COMPASSMOT_STATUS, MAVLINK_MESSAGE_INFO_AHRS2, MAVLINK_MESSAGE_INFO_CAMERA_STATUS, MAVLINK_MESSAGE_INFO_CAMERA_FEEDBACK, MAVLINK_MESSAGE_INFO_BATTERY2, MAVLINK_MESSAGE_INFO_AHRS3, MAVLINK_MESSAGE_INFO_AUTOPILOT_VERSION_REQUEST, MAVLINK_MESSAGE_INFO_REMOTE_LOG_DATA_BLOCK, MAVLINK_MESSAGE_INFO_REMOTE_LOG_BLOCK_STATUS, MAVLINK_MESSAGE_INFO_LED_CONTROL, MAVLINK_MESSAGE_INFO_MAG_CAL_PROGRESS, MAVLINK_MESSAGE_INFO_MAG_CAL_REPORT, MAVLINK_MESSAGE_INFO_EKF_STATUS_REPORT, MAVLINK_MESSAGE_INFO_PID_TUNING, MAVLINK_MESSAGE_INFO_DEEPSTALL, MAVLINK_MESSAGE_INFO_GIMBAL_REPORT, MAVLINK_MESSAGE_INFO_GIMBAL_CONTROL, MAVLINK_MESSAGE_INFO_GIMBAL_TORQUE_CMD_REPORT, MAVLINK_MESSAGE_INFO_GOPRO_HEARTBEAT, MAVLINK_MESSAGE_INFO_GOPRO_GET_REQUEST, MAVLINK_MESSAGE_INFO_GOPRO_GET_RESPONSE, MAVLINK_MESSAGE_INFO_GOPRO_SET_REQUEST, MAVLINK_MESSAGE_INFO_GOPRO_SET_RESPONSE, MAVLINK_MESSAGE_INFO_EFI_STATUS, MAVLINK_MESSAGE_INFO_RPM, MAVLINK_MESSAGE_INFO_ESTIMATOR_STATUS, MAVLINK_MESSAGE_INFO_WIND_COV, MAVLINK_MESSAGE_INFO_GPS_INPUT, MAVLINK_MESSAGE_INFO_GPS_RTCM_DATA, MAVLINK_MESSAGE_INFO_HIGH_LATENCY, MAVLINK_MESSAGE_INFO_HIGH_LATENCY2, MAVLINK_MESSAGE_INFO_VIBRATION, MAVLINK_MESSAGE_INFO_HOME_POSITION, MAVLINK_MESSAGE_INFO_SET_HOME_POSITION, MAVLINK_MESSAGE_INFO_MESSAGE_INTERVAL, MAVLINK_MESSAGE_INFO_EXTENDED_SYS_STATE, MAVLINK_MESSAGE_INFO_ADSB_VEHICLE, MAVLINK_MESSAGE_INFO_COLLISION, MAVLINK_MESSAGE_INFO_V2_EXTENSION, MAVLINK_MESSAGE_INFO_MEMORY_VECT, MAVLINK_MESSAGE_INFO_DEBUG_VECT, MAVLINK_MESSAGE_INFO_NAMED_VALUE_FLOAT, MAVLINK_MESSAGE_INFO_NAMED_VALUE_INT, MAVLINK_MESSAGE_INFO_STATUSTEXT, MAVLINK_MESSAGE_INFO_DEBUG, MAVLINK_MESSAGE_INFO_SETUP_SIGNING, MAVLINK_MESSAGE_INFO_BUTTON_CHANGE, MAVLINK_MESSAGE_INFO_PLAY_TUNE, MAVLINK_MESSAGE_INFO_CAMERA_INFORMATION, MAVLINK_MESSAGE_INFO_CAMERA_SETTINGS, MAVLINK_MESSAGE_INFO_STORAGE_INFORMATION, MAVLINK_MESSAGE_INFO_CAMERA_CAPTURE_STATUS, MAVLINK_MESSAGE_INFO_CAMERA_IMAGE_CAPTURED, MAVLINK_MESSAGE_INFO_FLIGHT_INFORMATION, MAVLINK_MESSAGE_INFO_MOUNT_ORIENTATION, MAVLINK_MESSAGE_INFO_LOGGING_DATA, MAVLINK_MESSAGE_INFO_LOGGING_DATA_ACKED, MAVLINK_MESSAGE_INFO_LOGGING_ACK, MAVLINK_MESSAGE_INFO_VIDEO_STREAM_INFORMATION, MAVLINK_MESSAGE_INFO_VIDEO_STREAM_STATUS, MAVLINK_MESSAGE_INFO_CAMERA_FOV_STATUS, MAVLINK_MESSAGE_INFO_CAMERA_TRACKING_IMAGE_STATUS, MAVLINK_MESSAGE_INFO_CAMERA_TRACKING_GEO_STATUS, MAVLINK_MESSAGE_INFO_CAMERA_THERMAL_RANGE, MAVLINK_MESSAGE_INFO_GIMBAL_MANAGER_INFORMATION, MAVLINK_MESSAGE_INFO_GIMBAL_MANAGER_STATUS, MAVLINK_MESSAGE_INFO_GIMBAL_MANAGER_SET_ATTITUDE, MAVLINK_MESSAGE_INFO_GIMBAL_DEVICE_INFORMATION, MAVLINK_MESSAGE_INFO_GIMBAL_DEVICE_SET_ATTITUDE, MAVLINK_MESSAGE_INFO_GIMBAL_DEVICE_ATTITUDE_STATUS, MAVLINK_MESSAGE_INFO_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE, MAVLINK_MESSAGE_INFO_GIMBAL_MANAGER_SET_PITCHYAW, MAVLINK_MESSAGE_INFO_GIMBAL_MANAGER_SET_MANUAL_CONTROL, MAVLINK_MESSAGE_INFO_ESC_INFO, MAVLINK_MESSAGE_INFO_ESC_STATUS, MAVLINK_MESSAGE_INFO_AIRSPEED, MAVLINK_MESSAGE_INFO_WIFI_CONFIG_AP, MAVLINK_MESSAGE_INFO_PROTOCOL_VERSION, MAVLINK_MESSAGE_INFO_AIS_VESSEL, MAVLINK_MESSAGE_INFO_UAVCAN_NODE_STATUS, MAVLINK_MESSAGE_INFO_UAVCAN_NODE_INFO, MAVLINK_MESSAGE_INFO_PARAM_EXT_REQUEST_READ, MAVLINK_MESSAGE_INFO_PARAM_EXT_REQUEST_LIST, MAVLINK_MESSAGE_INFO_PARAM_EXT_VALUE, MAVLINK_MESSAGE_INFO_PARAM_EXT_SET, MAVLINK_MESSAGE_INFO_PARAM_EXT_ACK, MAVLINK_MESSAGE_INFO_OBSTACLE_DISTANCE, MAVLINK_MESSAGE_INFO_ODOMETRY, MAVLINK_MESSAGE_INFO_TRAJECTORY_REPRESENTATION_WAYPOINTS, MAVLINK_MESSAGE_INFO_TRAJECTORY_REPRESENTATION_BEZIER, MAVLINK_MESSAGE_INFO_CELLULAR_STATUS, MAVLINK_MESSAGE_INFO_ISBD_LINK_STATUS, MAVLINK_MESSAGE_INFO_CELLULAR_CONFIG, MAVLINK_MESSAGE_INFO_RAW_RPM, MAVLINK_MESSAGE_INFO_UTM_GLOBAL_POSITION, MAVLINK_MESSAGE_INFO_PARAM_ERROR, MAVLINK_MESSAGE_INFO_DEBUG_FLOAT_ARRAY, MAVLINK_MESSAGE_INFO_ORBIT_EXECUTION_STATUS, MAVLINK_MESSAGE_INFO_FIGURE_EIGHT_EXECUTION_STATUS, MAVLINK_MESSAGE_INFO_SMART_BATTERY_INFO, MAVLINK_MESSAGE_INFO_FUEL_STATUS, MAVLINK_MESSAGE_INFO_BATTERY_INFO, MAVLINK_MESSAGE_INFO_GENERATOR_STATUS, MAVLINK_MESSAGE_INFO_ACTUATOR_OUTPUT_STATUS, MAVLINK_MESSAGE_INFO_TIME_ESTIMATE_TO_TARGET, MAVLINK_MESSAGE_INFO_TUNNEL, MAVLINK_MESSAGE_INFO_CAN_FRAME, MAVLINK_MESSAGE_INFO_CANFD_FRAME, MAVLINK_MESSAGE_INFO_CAN_FILTER_MODIFY, MAVLINK_MESSAGE_INFO_ONBOARD_COMPUTER_STATUS, MAVLINK_MESSAGE_INFO_COMPONENT_INFORMATION, MAVLINK_MESSAGE_INFO_COMPONENT_INFORMATION_BASIC, MAVLINK_MESSAGE_INFO_COMPONENT_METADATA, MAVLINK_MESSAGE_INFO_PLAY_TUNE_V2, MAVLINK_MESSAGE_INFO_SUPPORTED_TUNES, MAVLINK_MESSAGE_INFO_EVENT, MAVLINK_MESSAGE_INFO_CURRENT_EVENT_SEQUENCE, MAVLINK_MESSAGE_INFO_REQUEST_EVENT, MAVLINK_MESSAGE_INFO_RESPONSE_EVENT_ERROR, MAVLINK_MESSAGE_INFO_AVAILABLE_MODES, MAVLINK_MESSAGE_INFO_CURRENT_MODE, MAVLINK_MESSAGE_INFO_AVAILABLE_MODES_MONITOR, MAVLINK_MESSAGE_INFO_ILLUMINATOR_STATUS, MAVLINK_MESSAGE_INFO_WHEEL_DISTANCE, MAVLINK_MESSAGE_INFO_WINCH_STATUS, MAVLINK_MESSAGE_INFO_UAVIONIX_ADSB_OUT_CFG, MAVLINK_MESSAGE_INFO_UAVIONIX_ADSB_OUT_DYNAMIC, MAVLINK_MESSAGE_INFO_UAVIONIX_ADSB_TRANSCEIVER_HEALTH_REPORT, MAVLINK_MESSAGE_INFO_UAVIONIX_ADSB_OUT_CFG_REGISTRATION, MAVLINK_MESSAGE_INFO_UAVIONIX_ADSB_OUT_CFG_FLIGHTID, MAVLINK_MESSAGE_INFO_UAVIONIX_ADSB_GET, MAVLINK_MESSAGE_INFO_UAVIONIX_ADSB_OUT_CONTROL, MAVLINK_MESSAGE_INFO_UAVIONIX_ADSB_OUT_STATUS, MAVLINK_MESSAGE_INFO_LOWEHEISER_GOV_EFI, MAVLINK_MESSAGE_INFO_DEVICE_OP_READ, MAVLINK_MESSAGE_INFO_DEVICE_OP_READ_REPLY, MAVLINK_MESSAGE_INFO_DEVICE_OP_WRITE, MAVLINK_MESSAGE_INFO_DEVICE_OP_WRITE_REPLY, MAVLINK_MESSAGE_INFO_SECURE_COMMAND, MAVLINK_MESSAGE_INFO_SECURE_COMMAND_REPLY, MAVLINK_MESSAGE_INFO_ADAP_TUNING, MAVLINK_MESSAGE_INFO_VISION_POSITION_DELTA, MAVLINK_MESSAGE_INFO_AOA_SSA, MAVLINK_MESSAGE_INFO_ESC_TELEMETRY_1_TO_4, MAVLINK_MESSAGE_INFO_ESC_TELEMETRY_5_TO_8, MAVLINK_MESSAGE_INFO_ESC_TELEMETRY_9_TO_12, MAVLINK_MESSAGE_INFO_OSD_PARAM_CONFIG, MAVLINK_MESSAGE_INFO_OSD_PARAM_CONFIG_REPLY, MAVLINK_MESSAGE_INFO_OSD_PARAM_SHOW_CONFIG, MAVLINK_MESSAGE_INFO_OSD_PARAM_SHOW_CONFIG_REPLY, MAVLINK_MESSAGE_INFO_OBSTACLE_DISTANCE_3D, MAVLINK_MESSAGE_INFO_WATER_DEPTH, MAVLINK_MESSAGE_INFO_MCU_STATUS, MAVLINK_MESSAGE_INFO_ESC_TELEMETRY_13_TO_16, MAVLINK_MESSAGE_INFO_ESC_TELEMETRY_17_TO_20, MAVLINK_MESSAGE_INFO_ESC_TELEMETRY_21_TO_24, MAVLINK_MESSAGE_INFO_ESC_TELEMETRY_25_TO_28, MAVLINK_MESSAGE_INFO_ESC_TELEMETRY_29_TO_32, MAVLINK_MESSAGE_INFO_NAMED_VALUE_STRING, MAVLINK_MESSAGE_INFO_OPEN_DRONE_ID_BASIC_ID, MAVLINK_MESSAGE_INFO_OPEN_DRONE_ID_LOCATION, MAVLINK_MESSAGE_INFO_OPEN_DRONE_ID_AUTHENTICATION, MAVLINK_MESSAGE_INFO_OPEN_DRONE_ID_SELF_ID, MAVLINK_MESSAGE_INFO_OPEN_DRONE_ID_SYSTEM, MAVLINK_MESSAGE_INFO_OPEN_DRONE_ID_OPERATOR_ID, MAVLINK_MESSAGE_INFO_OPEN_DRONE_ID_MESSAGE_PACK, MAVLINK_MESSAGE_INFO_OPEN_DRONE_ID_ARM_STATUS, MAVLINK_MESSAGE_INFO_OPEN_DRONE_ID_SYSTEM_UPDATE, MAVLINK_MESSAGE_INFO_HYGROMETER_SENSOR, MAVLINK_MESSAGE_INFO_ICAROUS_HEARTBEAT, MAVLINK_MESSAGE_INFO_ICAROUS_KINEMATIC_BANDS, MAVLINK_MESSAGE_INFO_CUBEPILOT_RAW_RC, MAVLINK_MESSAGE_INFO_HERELINK_VIDEO_STREAM_INFORMATION, MAVLINK_MESSAGE_INFO_HERELINK_TELEM, MAVLINK_MESSAGE_INFO_CUBEPILOT_FIRMWARE_UPDATE_START, MAVLINK_MESSAGE_INFO_CUBEPILOT_FIRMWARE_UPDATE_RESP, MAVLINK_MESSAGE_INFO_AIRLINK_AUTH, MAVLINK_MESSAGE_INFO_AIRLINK_AUTH_RESPONSE}
# define MAVLINK_MESSAGE_NAMES {{ "ACTUATOR_CONTROL_TARGET", 140 }, { "ACTUATOR_OUTPUT_STATUS", 375 }, { "ADAP_TUNING", 11010 }, { "ADSB_VEHICLE", 246 }, { "AHRS", 163 }, { "AHRS2", 178 }, { "AHRS3", 182 }, { "AIRLINK_AUTH", 52000 }, { "AIRLINK_AUTH_RESPONSE", 52001 }, { "AIRSPEED", 295 }, { "AIRSPEED_AUTOCAL", 174 }, { "AIS_VESSEL", 301 }, { "ALTITUDE", 141 }, { "AOA_SSA", 11020 }, { "AP_ADC", 153 }, { "ATTITUDE", 30 }, { "ATTITUDE_QUATERNION", 31 }, { "ATTITUDE_QUATERNION_COV", 61 }, { "ATTITUDE_TARGET", 83 }, { "ATT_POS_MOCAP", 138 }, { "AUTH_KEY", 7 }, { "AUTOPILOT_STATE_FOR_GIMBAL_DEVICE", 286 }, { "AUTOPILOT_VERSION", 148 }, { "AUTOPILOT_VERSION_REQUEST", 183 }, { "AVAILABLE_MODES", 435 }, { "AVAILABLE_MODES_MONITOR", 437 }, { "BATTERY2", 181 }, { "BATTERY_INFO", 372 }, { "BATTERY_STATUS", 147 }, { "BUTTON_CHANGE", 257 }, { "CAMERA_CAPTURE_STATUS", 262 }, { "CAMERA_FEEDBACK", 180 }, { "CAMERA_FOV_STATUS", 271 }, { "CAMERA_IMAGE_CAPTURED", 263 }, { "CAMERA_INFORMATION", 259 }, { "CAMERA_SETTINGS", 260 }, { "CAMERA_STATUS", 179 }, { "CAMERA_THERMAL_RANGE", 277 }, { "CAMERA_TRACKING_GEO_STATUS", 276 }, { "CAMERA_TRACKING_IMAGE_STATUS", 275 }, { "CAMERA_TRIGGER", 112 }, { "CANFD_FRAME", 387 }, { "CAN_FILTER_MODIFY", 388 }, { "CAN_FRAME", 386 }, { "CELLULAR_CONFIG", 336 }, { "CELLULAR_STATUS", 334 }, { "CHANGE_OPERATOR_CONTROL", 5 }, { "CHANGE_OPERATOR_CONTROL_ACK", 6 }, { "COLLISION", 247 }, { "COMMAND_ACK", 77 }, { "COMMAND_CANCEL", 80 }, { "COMMAND_INT", 75 }, { "COMMAND_LONG", 76 }, { "COMPASSMOT_STATUS", 177 }, { "COMPONENT_INFORMATION", 395 }, { "COMPONENT_INFORMATION_BASIC", 396 }, { "COMPONENT_METADATA", 397 }, { "CONTROL_SYSTEM_STATE", 146 }, { "CUBEPILOT_FIRMWARE_UPDATE_RESP", 50005 }, { "CUBEPILOT_FIRMWARE_UPDATE_START", 50004 }, { "CUBEPILOT_RAW_RC", 50001 }, { "CURRENT_EVENT_SEQUENCE", 411 }, { "CURRENT_MODE", 436 }, { "DATA16", 169 }, { "DATA32", 170 }, { "DATA64", 171 }, { "DATA96", 172 }, { "DATA_STREAM", 67 }, { "DATA_TRANSMISSION_HANDSHAKE", 130 }, { "DEBUG", 254 }, { "DEBUG_FLOAT_ARRAY", 350 }, { "DEBUG_VECT", 250 }, { "DEEPSTALL", 195 }, { "DEVICE_OP_READ", 11000 }, { "DEVICE_OP_READ_REPLY", 11001 }, { "DEVICE_OP_WRITE", 11002 }, { "DEVICE_OP_WRITE_REPLY", 11003 }, { "DIGICAM_CONFIGURE", 154 }, { "DIGICAM_CONTROL", 155 }, { "DISTANCE_SENSOR", 132 }, { "EFI_STATUS", 225 }, { "EKF_STATUS_REPORT", 193 }, { "ENCAPSULATED_DATA", 131 }, { "ESC_INFO", 290 }, { "ESC_STATUS", 291 }, { "ESC_TELEMETRY_13_TO_16", 11040 }, { "ESC_TELEMETRY_17_TO_20", 11041 }, { "ESC_TELEMETRY_1_TO_4", 11030 }, { "ESC_TELEMETRY_21_TO_24", 11042 }, { "ESC_TELEMETRY_25_TO_28", 11043 }, { "ESC_TELEMETRY_29_TO_32", 11044 }, { "ESC_TELEMETRY_5_TO_8", 11031 }, { "ESC_TELEMETRY_9_TO_12", 11032 }, { "ESTIMATOR_STATUS", 230 }, { "EVENT", 410 }, { "EXTENDED_SYS_STATE", 245 }, { "FENCE_FETCH_POINT", 161 }, { "FENCE_POINT", 160 }, { "FENCE_STATUS", 162 }, { "FIGURE_EIGHT_EXECUTION_STATUS", 361 }, { "FILE_TRANSFER_PROTOCOL", 110 }, { "FLIGHT_INFORMATION", 264 }, { "FOLLOW_TARGET", 144 }, { "FUEL_STATUS", 371 }, { "GENERATOR_STATUS", 373 }, { "GIMBAL_CONTROL", 201 }, { "GIMBAL_DEVICE_ATTITUDE_STATUS", 285 }, { "GIMBAL_DEVICE_INFORMATION", 283 }, { "GIMBAL_DEVICE_SET_ATTITUDE", 284 }, { "GIMBAL_MANAGER_INFORMATION", 280 }, { "GIMBAL_MANAGER_SET_ATTITUDE", 282 }, { "GIMBAL_MANAGER_SET_MANUAL_CONTROL", 288 }, { "GIMBAL_MANAGER_SET_PITCHYAW", 287 }, { "GIMBAL_MANAGER_STATUS", 281 }, { "GIMBAL_REPORT", 200 }, { "GIMBAL_TORQUE_CMD_REPORT", 214 }, { "GLOBAL_POSITION_INT", 33 }, { "GLOBAL_POSITION_INT_COV", 63 }, { "GLOBAL_VISION_POSITION_ESTIMATE", 101 }, { "GOPRO_GET_REQUEST", 216 }, { "GOPRO_GET_RESPONSE", 217 }, { "GOPRO_HEARTBEAT", 215 }, { "GOPRO_SET_REQUEST", 218 }, { "GOPRO_SET_RESPONSE", 219 }, { "GPS2_RAW", 124 }, { "GPS2_RTK", 128 }, { "GPS_GLOBAL_ORIGIN", 49 }, { "GPS_INJECT_DATA", 123 }, { "GPS_INPUT", 232 }, { "GPS_RAW_INT", 24 }, { "GPS_RTCM_DATA", 233 }, { "GPS_RTK", 127 }, { "GPS_STATUS", 25 }, { "HEARTBEAT", 0 }, { "HERELINK_TELEM", 50003 }, { "HERELINK_VIDEO_STREAM_INFORMATION", 50002 }, { "HIGHRES_IMU", 105 }, { "HIGH_LATENCY", 234 }, { "HIGH_LATENCY2", 235 }, { "HIL_ACTUATOR_CONTROLS", 93 }, { "HIL_CONTROLS", 91 }, { "HIL_GPS", 113 }, { "HIL_OPTICAL_FLOW", 114 }, { "HIL_RC_INPUTS_RAW", 92 }, { "HIL_SENSOR", 107 }, { "HIL_STATE", 90 }, { "HIL_STATE_QUATERNION", 115 }, { "HOME_POSITION", 242 }, { "HWSTATUS", 165 }, { "HYGROMETER_SENSOR", 12920 }, { "ICAROUS_HEARTBEAT", 42000 }, { "ICAROUS_KINEMATIC_BANDS", 42001 }, { "ILLUMINATOR_STATUS", 440 }, { "ISBD_LINK_STATUS", 335 }, { "LANDING_TARGET", 149 }, { "LED_CONTROL", 186 }, { "LIMITS_STATUS", 167 }, { "LINK_NODE_STATUS", 8 }, { "LOCAL_POSITION_NED", 32 }, { "LOCAL_POSITION_NED_COV", 64 }, { "LOCAL_POSITION_NED_SYSTEM_GLOBAL_OFFSET", 89 }, { "LOGGING_ACK", 268 }, { "LOGGING_DATA", 266 }, { "LOGGING_DATA_ACKED", 267 }, { "LOG_DATA", 120 }, { "LOG_ENTRY", 118 }, { "LOG_ERASE", 121 }, { "LOG_REQUEST_DATA", 119 }, { "LOG_REQUEST_END", 122 }, { "LOG_REQUEST_LIST", 117 }, { "LOWEHEISER_GOV_EFI", 10151 }, { "MAG_CAL_PROGRESS", 191 }, { "MAG_CAL_REPORT", 192 }, { "MANUAL_CONTROL", 69 }, { "MANUAL_SETPOINT", 81 }, { "MCU_STATUS", 11039 }, { "MEMINFO", 152 }, { "MEMORY_VECT", 249 }, { "MESSAGE_INTERVAL", 244 }, { "MISSION_ACK", 47 }, { "MISSION_CLEAR_ALL", 45 }, { "MISSION_COUNT", 44 }, { "MISSION_CURRENT", 42 }, { "MISSION_ITEM", 39 }, { "MISSION_ITEM_INT", 73 }, { "MISSION_ITEM_REACHED", 46 }, { "MISSION_REQUEST", 40 }, { "MISSION_REQUEST_INT", 51 }, { "MISSION_REQUEST_LIST", 43 }, { "MISSION_REQUEST_PARTIAL_LIST", 37 }, { "MISSION_SET_CURRENT", 41 }, { "MISSION_WRITE_PARTIAL_LIST", 38 }, { "MOUNT_CONFIGURE", 156 }, { "MOUNT_CONTROL", 157 }, { "MOUNT_ORIENTATION", 265 }, { "MOUNT_STATUS", 158 }, { "NAMED_VALUE_FLOAT", 251 }, { "NAMED_VALUE_INT", 252 }, { "NAMED_VALUE_STRING", 11060 }, { "NAV_CONTROLLER_OUTPUT", 62 }, { "OBSTACLE_DISTANCE", 330 }, { "OBSTACLE_DISTANCE_3D", 11037 }, { "ODOMETRY", 331 }, { "ONBOARD_COMPUTER_STATUS", 390 }, { "OPEN_DRONE_ID_ARM_STATUS", 12918 }, { "OPEN_DRONE_ID_AUTHENTICATION", 12902 }, { "OPEN_DRONE_ID_BASIC_ID", 12900 }, { "OPEN_DRONE_ID_LOCATION", 12901 }, { "OPEN_DRONE_ID_MESSAGE_PACK", 12915 }, { "OPEN_DRONE_ID_OPERATOR_ID", 12905 }, { "OPEN_DRONE_ID_SELF_ID", 12903 }, { "OPEN_DRONE_ID_SYSTEM", 12904 }, { "OPEN_DRONE_ID_SYSTEM_UPDATE", 12919 }, { "OPTICAL_FLOW", 100 }, { "OPTICAL_FLOW_RAD", 106 }, { "ORBIT_EXECUTION_STATUS", 360 }, { "OSD_PARAM_CONFIG", 11033 }, { "OSD_PARAM_CONFIG_REPLY", 11034 }, { "OSD_PARAM_SHOW_CONFIG", 11035 }, { "OSD_PARAM_SHOW_CONFIG_REPLY", 11036 }, { "PARAM_ERROR", 345 }, { "PARAM_EXT_ACK", 324 }, { "PARAM_EXT_REQUEST_LIST", 321 }, { "PARAM_EXT_REQUEST_READ", 320 }, { "PARAM_EXT_SET", 323 }, { "PARAM_EXT_VALUE", 322 }, { "PARAM_MAP_RC", 50 }, { "PARAM_REQUEST_LIST", 21 }, { "PARAM_REQUEST_READ", 20 }, { "PARAM_SET", 23 }, { "PARAM_VALUE", 22 }, { "PID_TUNING", 194 }, { "PING", 4 }, { "PLAY_TUNE", 258 }, { "PLAY_TUNE_V2", 400 }, { "POSITION_TARGET_GLOBAL_INT", 87 }, { "POSITION_TARGET_LOCAL_NED", 85 }, { "POWER_STATUS", 125 }, { "PROTOCOL_VERSION", 300 }, { "RADIO", 166 }, { "RADIO_STATUS", 109 }, { "RALLY_FETCH_POINT", 176 }, { "RALLY_POINT", 175 }, { "RANGEFINDER", 173 }, { "RAW_IMU", 27 }, { "RAW_PRESSURE", 28 }, { "RAW_RPM", 339 }, { "RC_CHANNELS", 65 }, { "RC_CHANNELS_OVERRIDE", 70 }, { "RC_CHANNELS_RAW", 35 }, { "RC_CHANNELS_SCALED", 34 }, { "REMOTE_LOG_BLOCK_STATUS", 185 }, { "REMOTE_LOG_DATA_BLOCK", 184 }, { "REQUEST_DATA_STREAM", 66 }, { "REQUEST_EVENT", 412 }, { "RESOURCE_REQUEST", 142 }, { "RESPONSE_EVENT_ERROR", 413 }, { "RPM", 226 }, { "SAFETY_ALLOWED_AREA", 55 }, { "SAFETY_SET_ALLOWED_AREA", 54 }, { "SCALED_IMU", 26 }, { "SCALED_IMU2", 116 }, { "SCALED_IMU3", 129 }, { "SCALED_PRESSURE", 29 }, { "SCALED_PRESSURE2", 137 }, { "SCALED_PRESSURE3", 143 }, { "SECURE_COMMAND", 11004 }, { "SECURE_COMMAND_REPLY", 11005 }, { "SENSOR_OFFSETS", 150 }, { "SERIAL_CONTROL", 126 }, { "SERVO_OUTPUT_RAW", 36 }, { "SETUP_SIGNING", 256 }, { "SET_ACTUATOR_CONTROL_TARGET", 139 }, { "SET_ATTITUDE_TARGET", 82 }, { "SET_GPS_GLOBAL_ORIGIN", 48 }, { "SET_HOME_POSITION", 243 }, { "SET_MAG_OFFSETS", 151 }, { "SET_MODE", 11 }, { "SET_POSITION_TARGET_GLOBAL_INT", 86 }, { "SET_POSITION_TARGET_LOCAL_NED", 84 }, { "SIMSTATE", 164 }, { "SIM_STATE", 108 }, { "SMART_BATTERY_INFO", 370 }, { "STATUSTEXT", 253 }, { "STORAGE_INFORMATION", 261 }, { "SUPPORTED_TUNES", 401 }, { "SYSTEM_TIME", 2 }, { "SYS_STATUS", 1 }, { "TERRAIN_CHECK", 135 }, { "TERRAIN_DATA", 134 }, { "TERRAIN_REPORT", 136 }, { "TERRAIN_REQUEST", 133 }, { "TIMESYNC", 111 }, { "TIME_ESTIMATE_TO_TARGET", 380 }, { "TRAJECTORY_REPRESENTATION_BEZIER", 333 }, { "TRAJECTORY_REPRESENTATION_WAYPOINTS", 332 }, { "TUNNEL", 385 }, { "UAVCAN_NODE_INFO", 311 }, { "UAVCAN_NODE_STATUS", 310 }, { "UAVIONIX_ADSB_GET", 10006 }, { "UAVIONIX_ADSB_OUT_CFG", 10001 }, { "UAVIONIX_ADSB_OUT_CFG_FLIGHTID", 10005 }, { "UAVIONIX_ADSB_OUT_CFG_REGISTRATION", 10004 }, { "UAVIONIX_ADSB_OUT_CONTROL", 10007 }, { "UAVIONIX_ADSB_OUT_DYNAMIC", 10002 }, { "UAVIONIX_ADSB_OUT_STATUS", 10008 }, { "UAVIONIX_ADSB_TRANSCEIVER_HEALTH_REPORT", 10003 }, { "UTM_GLOBAL_POSITION", 340 }, { "V2_EXTENSION", 248 }, { "VFR_HUD", 74 }, { "VIBRATION", 241 }, { "VICON_POSITION_ESTIMATE", 104 }, { "VIDEO_STREAM_INFORMATION", 269 }, { "VIDEO_STREAM_STATUS", 270 }, { "VISION_POSITION_DELTA", 11011 }, { "VISION_POSITION_ESTIMATE", 102 }, { "VISION_SPEED_ESTIMATE", 103 }, { "WATER_DEPTH", 11038 }, { "WHEEL_DISTANCE", 9000 }, { "WIFI_CONFIG_AP", 299 }, { "WINCH_STATUS", 9005 }, { "WIND", 168 }, { "WIND_COV", 231 }}
# if MAVLINK_COMMAND_24BIT
#  include "../mavlink_get_info.h"
# endif
#endif

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // MAVLINK_ARDUPILOTMEGA_H
