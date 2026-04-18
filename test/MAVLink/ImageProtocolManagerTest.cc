#include "ImageProtocolManagerTest.h"

#include "ImageProtocolManager.h"
#include "MAVLinkLib.h"

#include <algorithm>
#include <cstring>

#include <QtCore/QLoggingCategory>
#include <QtCore/QRegularExpression>
#include <QtGui/QImage>
#include <QtTest/QSignalSpy>

namespace {

constexpr uint8_t kSystemId    = 1;
constexpr uint8_t kComponentId = MAV_COMP_ID_AUTOPILOT1;
constexpr uint8_t kChannel     = MAVLINK_COMM_0;

// Build a DATA_TRANSMISSION_HANDSHAKE message describing an upcoming image.
mavlink_message_t makeHandshake(uint8_t type, uint32_t size, uint16_t width,
                                uint16_t height, uint16_t packets, uint8_t payload,
                                uint8_t jpgQuality = 50)
{
    const mavlink_data_transmission_handshake_t data{
        /*size*/    size,
        /*width*/   width,
        /*height*/  height,
        /*packets*/ packets,
        /*type*/    type,
        /*payload*/ payload,
        /*jpg_quality*/ jpgQuality,
    };
    mavlink_message_t msg{};
    (void) mavlink_msg_data_transmission_handshake_encode_chan(
        kSystemId, kComponentId, kChannel, &msg, &data);
    return msg;
}

// Build an ENCAPSULATED_DATA packet with the given sequence number and payload.
mavlink_message_t makeEncapsulated(uint16_t seqnr, const uint8_t* bytes, size_t len)
{
    mavlink_encapsulated_data_t data{};
    data.seqnr = seqnr;
    const size_t cap = std::min(len, sizeof(data.data));
    if (cap > 0 && bytes != nullptr) {
        std::memcpy(data.data, bytes, cap);
    }
    mavlink_message_t msg{};
    (void) mavlink_msg_encapsulated_data_encode_chan(
        kSystemId, kComponentId, kChannel, &msg, &data);
    return msg;
}

} // namespace

// ---------------------------------------------------------------------------
// Initial state
// ---------------------------------------------------------------------------

void ImageProtocolManagerTest::_testInitialState()
{
    ImageProtocolManager manager;
    QCOMPARE(manager.flowImageIndex(), 0u);
}

// ---------------------------------------------------------------------------
// requestImage / cancelRequest
// ---------------------------------------------------------------------------

void ImageProtocolManagerTest::_testRequestImageEncodesHandshake()
{
    ImageProtocolManager manager;

    mavlink_message_t msg{};
    QVERIFY(manager.requestImage(kSystemId, kComponentId, kChannel, msg));
    QCOMPARE(msg.msgid, static_cast<uint32_t>(MAVLINK_MSG_ID_DATA_TRANSMISSION_HANDSHAKE));
    QCOMPARE(msg.sysid, kSystemId);
    QCOMPARE(msg.compid, kComponentId);

    // The encoded handshake should be a JPEG request with quality 50.
    mavlink_data_transmission_handshake_t decoded{};
    mavlink_msg_data_transmission_handshake_decode(&msg, &decoded);
    QCOMPARE(decoded.type, static_cast<uint8_t>(MAVLINK_DATA_STREAM_IMG_JPEG));
    QCOMPARE(decoded.jpg_quality, static_cast<uint8_t>(50));
}

void ImageProtocolManagerTest::_testRequestImageRejectedWhileInProgress()
{
    ImageProtocolManager manager;

    // Prime an in-progress transmission via a handshake.
    constexpr uint8_t kPayload = 50;
    constexpr uint32_t kImageSize = 200;
    const mavlink_message_t hs = makeHandshake(MAVLINK_DATA_STREAM_IMG_JPEG,
                                               kImageSize, 10, 20, /*packets*/4, kPayload);
    manager.mavlinkMessageReceived(hs);

    // Second requestImage call while a transmission is pending is rejected.
    mavlink_message_t msg{};
    QVERIFY(!manager.requestImage(kSystemId, kComponentId, kChannel, msg));
}

void ImageProtocolManagerTest::_testCancelRequestEncodesZeroedHandshake()
{
    ImageProtocolManager manager;

    mavlink_message_t msg{};
    manager.cancelRequest(kSystemId, kComponentId, kChannel, msg);
    QCOMPARE(msg.msgid, static_cast<uint32_t>(MAVLINK_MSG_ID_DATA_TRANSMISSION_HANDSHAKE));

    mavlink_data_transmission_handshake_t decoded{};
    mavlink_msg_data_transmission_handshake_decode(&msg, &decoded);
    QCOMPARE(decoded.size,    0u);
    QCOMPARE(decoded.packets, static_cast<uint16_t>(0));
    QCOMPARE(decoded.payload, static_cast<uint8_t>(0));
}

// ---------------------------------------------------------------------------
// Handshake validation / rejection paths
// ---------------------------------------------------------------------------

void ImageProtocolManagerTest::_testRejectHandshakeWithZeroFields()
{
    ImageProtocolManager manager;

    // All-zero handshake is silently ignored (warning is logged).
    QTest::ignoreMessage(QtWarningMsg,
                         QRegularExpression(QStringLiteral("Invalid field")));

    const mavlink_message_t hs = makeHandshake(MAVLINK_DATA_STREAM_IMG_JPEG,
                                               /*size*/0, 10, 20, /*packets*/0, /*payload*/0);
    manager.mavlinkMessageReceived(hs);

    // Follow-up data must be rejected because no transmission is active.
    QTest::ignoreMessage(QtWarningMsg,
                         QRegularExpression(QStringLiteral("no prior DATA_TRANSMISSION_HANDSHAKE")));
    const uint8_t buf[4] = {1, 2, 3, 4};
    const mavlink_message_t ed = makeEncapsulated(0, buf, sizeof(buf));
    manager.mavlinkMessageReceived(ed);

    // Index never advanced because imageReady was never emitted.
    QCOMPARE(manager.flowImageIndex(), 0u);
}

void ImageProtocolManagerTest::_testRejectHandshakeExceedingMaxImageSize()
{
    ImageProtocolManager manager;

    // 2 MB exceeds the internal 1 MB cap.
    constexpr uint32_t kOversizedImage = 2u * 1024u * 1024u;
    QTest::ignoreMessage(QtWarningMsg,
                         QRegularExpression(QStringLiteral("Image size exceeds limit")));
    const mavlink_message_t hs = makeHandshake(MAVLINK_DATA_STREAM_IMG_JPEG,
                                               kOversizedImage, 1024, 1024,
                                               /*packets*/10, /*payload*/253);
    manager.mavlinkMessageReceived(hs);

    QCOMPARE(manager.flowImageIndex(), 0u);
}

void ImageProtocolManagerTest::_testRejectHandshakeWithOversizedPayload()
{
    ImageProtocolManager manager;

    // payload larger than the ENCAPSULATED_DATA field capacity (253) is
    // rejected to avoid buffer overruns on decode.
    const uint8_t kBogusPayload = 255;
    QTest::ignoreMessage(QtWarningMsg,
                         QRegularExpression(QStringLiteral("payload exceeds")));
    const mavlink_message_t hs = makeHandshake(MAVLINK_DATA_STREAM_IMG_JPEG,
                                               /*size*/500, 10, 20,
                                               /*packets*/2, kBogusPayload);
    manager.mavlinkMessageReceived(hs);

    QCOMPARE(manager.flowImageIndex(), 0u);
}

// ---------------------------------------------------------------------------
// Data packet edge cases
// ---------------------------------------------------------------------------

void ImageProtocolManagerTest::_testEncapsulatedDataWithoutHandshakeIgnored()
{
    ImageProtocolManager manager;

    QSignalSpy readySpy(&manager, &ImageProtocolManager::imageReady);

    QTest::ignoreMessage(QtWarningMsg,
                         QRegularExpression(QStringLiteral("no prior DATA_TRANSMISSION_HANDSHAKE")));
    const uint8_t buf[4] = {9, 9, 9, 9};
    const mavlink_message_t ed = makeEncapsulated(0, buf, sizeof(buf));
    manager.mavlinkMessageReceived(ed);

    QCOMPARE(readySpy.count(), 0);
    QCOMPARE(manager.flowImageIndex(), 0u);
}

void ImageProtocolManagerTest::_testEncapsulatedDataPastEndIgnored()
{
    ImageProtocolManager manager;

    constexpr uint8_t  kPayload   = 50;
    constexpr uint16_t kPackets   = 2;
    constexpr uint32_t kImageSize = kPayload * kPackets; // 100 bytes
    const mavlink_message_t hs = makeHandshake(MAVLINK_DATA_STREAM_IMG_JPEG,
                                               kImageSize, 10, 10, kPackets, kPayload);
    manager.mavlinkMessageReceived(hs);

    // seqnr=5 → bytePosition = 5 * 50 = 250, past the 100-byte image.
    QTest::ignoreMessage(QtWarningMsg,
                         QRegularExpression(QStringLiteral("past end of image size")));
    uint8_t buf[kPayload] = {};
    const mavlink_message_t edBad = makeEncapsulated(5, buf, sizeof(buf));
    manager.mavlinkMessageReceived(edBad);

    // No image was emitted — transmission is still incomplete.
    QCOMPARE(manager.flowImageIndex(), 0u);
}

// ---------------------------------------------------------------------------
// End-to-end: reassemble a complete image
// ---------------------------------------------------------------------------

void ImageProtocolManagerTest::_testCompleteImageEmitsImageReadyAndIndex()
{
    ImageProtocolManager manager;

    QSignalSpy readySpy(&manager, &ImageProtocolManager::imageReady);
    QSignalSpy indexSpy(&manager, &ImageProtocolManager::flowImageIndexChanged);

    // Use RAW8U: ImageProtocolManager synthesizes a PGM P5 header from
    // width/height so any raw pixel payload will load as a valid QImage.
    constexpr uint16_t kWidth   = 4;
    constexpr uint16_t kHeight  = 4;
    constexpr uint32_t kSize    = kWidth * kHeight;   // 16 bytes of grayscale
    constexpr uint8_t  kPayload = 8;                  // 2 packets
    constexpr uint16_t kPackets = kSize / kPayload;   // = 2

    const mavlink_message_t hs = makeHandshake(MAVLINK_DATA_STREAM_IMG_RAW8U,
                                               kSize, kWidth, kHeight, kPackets, kPayload);
    manager.mavlinkMessageReceived(hs);

    const uint8_t first[kPayload]  = { 10, 20, 30, 40, 50, 60, 70, 80 };
    const uint8_t second[kPayload] = { 90, 100, 110, 120, 130, 140, 150, 160 };

    manager.mavlinkMessageReceived(makeEncapsulated(0, first,  sizeof(first)));
    QCOMPARE(readySpy.count(), 0);  // not yet complete

    manager.mavlinkMessageReceived(makeEncapsulated(1, second, sizeof(second)));
    QCOMPARE(readySpy.count(), 1);
    QCOMPARE(indexSpy.count(), 1);
    QCOMPARE(indexSpy.takeFirst().at(0).toUInt(), 1u);
    QCOMPARE(manager.flowImageIndex(), 1u);

    const QImage image = readySpy.takeFirst().at(0).value<QImage>();
    QVERIFY(!image.isNull());
    QCOMPARE(image.width(),  static_cast<int>(kWidth));
    QCOMPARE(image.height(), static_cast<int>(kHeight));
}

void ImageProtocolManagerTest::_testMultipleImagesIncrementIndexIndependently()
{
    ImageProtocolManager manager;

    QSignalSpy readySpy(&manager, &ImageProtocolManager::imageReady);

    constexpr uint16_t kWidth   = 2;
    constexpr uint16_t kHeight  = 2;
    constexpr uint32_t kSize    = kWidth * kHeight;  // 4 bytes
    constexpr uint8_t  kPayload = 4;                 // single-packet images
    constexpr uint16_t kPackets = 1;

    // Transmit three single-packet images back-to-back.
    for (uint32_t i = 0; i < 3; ++i) {
        const mavlink_message_t hs = makeHandshake(MAVLINK_DATA_STREAM_IMG_RAW8U,
                                                   kSize, kWidth, kHeight, kPackets, kPayload);
        manager.mavlinkMessageReceived(hs);

        const uint8_t pixels[kPayload] = {
            static_cast<uint8_t>(i * 4 + 0),
            static_cast<uint8_t>(i * 4 + 1),
            static_cast<uint8_t>(i * 4 + 2),
            static_cast<uint8_t>(i * 4 + 3),
        };
        manager.mavlinkMessageReceived(makeEncapsulated(0, pixels, sizeof(pixels)));
    }

    QCOMPARE(readySpy.count(), 3);
    QCOMPARE(manager.flowImageIndex(), 3u);
}

// ---------------------------------------------------------------------------
// Unrelated messages
// ---------------------------------------------------------------------------

void ImageProtocolManagerTest::_testUnrelatedMessageIdIgnored()
{
    ImageProtocolManager manager;

    QSignalSpy readySpy(&manager, &ImageProtocolManager::imageReady);

    // HEARTBEAT is not handled by ImageProtocolManager and must be a no-op.
    mavlink_heartbeat_t hb{};
    hb.type           = MAV_TYPE_QUADROTOR;
    hb.autopilot      = MAV_AUTOPILOT_PX4;
    hb.base_mode      = 0;
    hb.custom_mode    = 0;
    hb.system_status  = MAV_STATE_STANDBY;
    hb.mavlink_version = MAVLINK_VERSION;

    mavlink_message_t hbMsg{};
    (void) mavlink_msg_heartbeat_encode_chan(kSystemId, kComponentId, kChannel, &hbMsg, &hb);
    manager.mavlinkMessageReceived(hbMsg);

    QCOMPARE(readySpy.count(), 0);
    QCOMPARE(manager.flowImageIndex(), 0u);
}

UT_REGISTER_TEST(ImageProtocolManagerTest, TestLabel::Unit)
