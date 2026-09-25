#pragma once
#include <cstddef>
#include <cstdint>

#define FEMTO_MSG_ID_GPGGA 218

/*** femtomes protocol binary message definitions ***/

namespace Femto {
inline constexpr size_t HEADER_SIZE = 28;
}  // namespace Femto

struct femto_msg_t
{
    uint8_t data[600]{};
    uint32_t crc = 0;
    uint8_t header[Femto::HEADER_SIZE]{};
    uint16_t messageId = 0;
    uint16_t payloadLength = 0;
    uint16_t read = 0;
};

/*** END OF femtomes protocol binary message and payload definitions ***/

enum class FemtoDecodeState
{
    pream_ble1,  /**< Frame header preamble first byte 0xaa */
    pream_ble2,  /**< Frame header preamble second byte 0x44 */
    pream_ble3,  /**< Frame header preamble third byte 0x12 */
    head_length, /**< Frame header length */
    head_data,   /**< Frame header data */
    data,        /**< Frame data */
    crc1,        /**< Frame crc1 */
    crc2,        /**< Frame crc2 */
    crc3,        /**< Frame crc3 */
    crc4,        /**< Frame crc4 */
};
