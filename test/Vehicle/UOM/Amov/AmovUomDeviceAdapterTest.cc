#include "AmovUomDeviceAdapterTest.h"

#include <cstring>
#include <functional>

#include <QtCore/QCryptographicHash>
#include <QtTest/QSignalSpy>

#include "AmovUomDeviceAdapter.h"
#include "MAVLinkLib.h"
#include "MAVLinkProtocol.h"
#include "MockLink.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

UT_REGISTER_TEST(AmovUomDeviceAdapterTest, TestLabel::Integration)

namespace {
class AmovActivationPeer : public QObject
{
public:
    AmovActivationPeer(MockLink* link, Vehicle* vehicle)
        : _link(link)
        , _vehicle(vehicle)
    {
        connect(link, &MockLink::writeBytesQueuedSignal, this, [this](const QByteArray& bytes) {
            mavlink_message_t message{};
            mavlink_status_t status{};
            for (const char byte : bytes) {
                if (mavlink_frame_char_buffer(&_rx, &_rxStatus, static_cast<uint8_t>(byte), &message, &status) ==
                        MAVLINK_FRAMING_OK &&
                    message.msgid == MAVLINK_MSG_ID_AMOV_DEVICE_REQUEST) {
                    _handle(message);
                }
            }
        });
    }

    void release()
    {
        hold = false;
        const auto responses = held;
        held.clear();
        for (const auto& message : responses) {
            _link->respondWithMavlinkMessage(message);
        }
    }

    QByteArray serial = "TESTAIRCRAFT00000001";
    QByteArray uid = QByteArray::fromHex("001122aabbcc");
    QByteArray nodes = QByteArray(1, char(125));
    quint32 boot = 7;
    quint32 revision = 12;
    quint8 active = 0;
    int mutations = 0;
    int receipts = 0;
    int receiptBusy = 0;
    bool timeoutMutation = false;
    bool omitDurability = false;
    bool wrongDigest = false;
    bool unknownReceipt = false;
    bool hold = false;
    bool foreignRepliesFirst = false;
    QString error;
    std::function<void()> afterMutation;
    QList<mavlink_message_t> held;

private:
    void _handle(const mavlink_message_t& message)
    {
        mavlink_amov_device_request_t envelope{};
        mavlink_msg_amov_device_request_decode(&message, &envelope);
        if (envelope.protocol_version != 1 || envelope.target_system != _vehicle->id() ||
            envelope.target_component != _vehicle->defaultComponentId() || message.magic != MAVLINK_STX) {
            error = "Invalid envelope or MAVLink version";
            return;
        }
        if (envelope.operation == 0) {
            _send(envelope, nodes);
            return;
        }
        amov_device::Request request;
        if (!amov_device::decode_request(envelope.payload, envelope.payload_length, request)) {
            error = "Invalid activation payload";
            return;
        }
        amov_device::Response response;
        response.operation = request.operation;
        response.transaction = request.transaction;
        response.boot = boot;
        response.revision = revision;
        response.activation_state = active;
        response.total = 3;
        std::memcpy(response.uid, uid.constData(), 6);
        if (request.operation == amov_device::Operation::INFO) {
            response.value_length = serial.size();
            std::memcpy(response.value, serial.constData(), serial.size());
        } else if (request.operation == amov_device::Operation::ACTIVATE) {
            if (request.boot != boot || request.revision != revision ||
                QByteArray(reinterpret_cast<char*>(request.uid), 6) != uid ||
                QByteArray(reinterpret_cast<char*>(request.value + 1), 20) != serial) {
                error = "Activation was not bound to current identity";
                return;
            }
            ++mutations;
            active = request.value[0];
            ++revision;
            response.revision = revision;
            response.activation_state = active;
            response.result = amov_device::Result::SAVED;
            response.flags = omitDurability ? 0 : amov_device::DURABLE;
            _receipt = response;
            _digest = QCryptographicHash::hash(
                QByteArrayView(reinterpret_cast<const char*>(envelope.payload), envelope.payload_length),
                QCryptographicHash::Blake2b_256);
            if (afterMutation) {
                afterMutation();
            }
            if (timeoutMutation) {
                _send(envelope, {}, 4);
                return;
            }
        } else if (request.operation == amov_device::Operation::STATUS) {
            ++receipts;
            if (receiptBusy > 0) {
                --receiptBusy;
                _send(envelope, {}, 3);
                return;
            }
            const QByteArray supplied(reinterpret_cast<char*>(request.value), request.value_length);
            if (wrongDigest || supplied != _digest) {
                response.result = amov_device::Result::CONFLICT;
            } else if (unknownReceipt || request.transaction != _receipt.transaction) {
                response.result = amov_device::Result::UNKNOWN;
            } else {
                response = _receipt;
                response.boot = boot;
            }
        }
        uint8_t bytes[amov_device::RESPONSE_SIZE]{};
        if (!amov_device::encode_response(response, bytes, sizeof(bytes))) {
            error = "Invalid test response";
            return;
        }
        _send(envelope, QByteArray(reinterpret_cast<char*>(bytes), sizeof(bytes)));
    }

    void _send(const mavlink_amov_device_request_t& request, const QByteArray& payload, quint8 status = 0)
    {
        mavlink_amov_device_response_t response{};
        response.session_id = request.session_id;
        response.request_id = request.request_id;
        response.target_system = MAVLinkProtocol::instance()->getSystemId();
        response.target_component = MAVLinkProtocol::getComponentId();
        response.protocol_version = 1;
        response.operation = request.operation;
        response.node_id = request.node_id;
        response.status = status;
        response.payload_length = payload.size();
        std::memcpy(response.payload, payload.constData(), payload.size());
        const auto send = [this](const mavlink_amov_device_response_t& value, int system) {
            mavlink_message_t message{};
            mavlink_msg_amov_device_response_encode(system, _vehicle->defaultComponentId(), &message, &value);
            if (hold) {
                held.append(message);
            } else {
                _link->respondWithMavlinkMessage(message);
            }
        };
        if (foreignRepliesFirst) {
            foreignRepliesFirst = false;
            auto foreign = response;
            ++foreign.session_id;
            send(foreign, _vehicle->id());
            foreign = response;
            --foreign.target_system;
            send(foreign, _vehicle->id());
            send(response, _vehicle->id() + 1);
        }
        send(response, _vehicle->id());
    }

    MockLink* _link;
    Vehicle* _vehicle;
    mavlink_message_t _rx{};
    mavlink_status_t _rxStatus{};
    amov_device::Response _receipt;
    QByteArray _digest;
};
}  // namespace

void AmovUomDeviceAdapterTest::_activationRoundTrip_data()
{
    QTest::addColumn<bool>("active");
    QTest::addColumn<bool>("recoverReceipt");
    QTest::newRow("activate") << true << false;
    QTest::newRow("deactivate") << false << false;
    QTest::newRow("activate-recover") << true << true;
    QTest::newRow("deactivate-recover") << false << true;
}

void AmovUomDeviceAdapterTest::_activationRoundTrip()
{
    QFETCH(bool, active);
    QFETCH(bool, recoverReceipt);
    AmovActivationPeer peer(mockLink(), vehicle());
    peer.active = active ? 0 : 1;
    peer.timeoutMutation = recoverReceipt;
    peer.receiptBusy = recoverReceipt ? 2 : 0;
    AmovUomDeviceAdapter adapter(vehicle());
    QSignalSpy read(&adapter, &UomDeviceAdapter::activationRead);
    QSignalSpy written(&adapter, &UomDeviceAdapter::activationWritten);
    QSignalSpy failed(&adapter, &UomDeviceAdapter::activationFailed);
    mavlink_get_channel_status(mockLink()->mavlinkChannel())->flags |= MAVLINK_STATUS_FLAG_OUT_MAVLINK1;
    adapter.refresh();
    QTRY_VERIFY_WITH_TIMEOUT(adapter.canWrite(), TestTimeout::mediumMs());
    adapter.readActivation(10);
    QTRY_COMPARE_WITH_TIMEOUT(read.count(), 1, TestTimeout::mediumMs());
    QCOMPARE(qvariant_cast<UomRegistrationDevice>(read[0][1]).state, active ? 0 : 1);
    adapter.setActivation(11, adapter.device(), active);
    QTRY_COMPARE_WITH_TIMEOUT(written.count(), 1, TestTimeout::mediumMs());
    QCOMPARE(written[0][0].toULongLong(), quint64(11));
    const auto saved = qvariant_cast<UomRegistrationDevice>(written[0][1]);
    QCOMPARE(saved.state, active ? 1 : 0);
    QCOMPARE(saved.revision, quint32(13));
    QCOMPARE(peer.mutations, 1);
    QVERIFY(peer.receipts >= 1);
    QCOMPARE(peer.error, QString());
    QCOMPARE(failed.count(), 0);
}

void AmovUomDeviceAdapterTest::_identityGuard_data()
{
    QTest::addColumn<QString>("field");
    QTest::addColumn<bool>("onDevice");
    for (const char* field : {"serial", "uid", "boot", "revision", "generation", "state"}) {
        QTest::newRow(qPrintable(QStringLiteral("stale-%1").arg(field))) << QString::fromLatin1(field) << false;
        if (std::strcmp(field, "generation")) {
            QTest::newRow(qPrintable(QStringLiteral("device-%1").arg(field))) << QString::fromLatin1(field) << true;
        }
    }
}

void AmovUomDeviceAdapterTest::_identityGuard()
{
    QFETCH(QString, field);
    QFETCH(bool, onDevice);
    AmovActivationPeer peer(mockLink(), vehicle());
    AmovUomDeviceAdapter adapter(vehicle());
    adapter.refresh();
    QTRY_VERIFY_WITH_TIMEOUT(adapter.canWrite(), TestTimeout::mediumMs());
    auto expected = adapter.device();
    if (field == "serial") {
        if (onDevice) {
            peer.serial[0] = 'X';
        } else {
            expected.serial[0] = u'X';
        }
    } else if (field == "uid") {
        if (onDevice) {
            peer.uid[0] = 'X';
        } else {
            expected.uid[0] = u'F';
        }
    } else if (field == "boot") {
        if (onDevice) {
            ++peer.boot;
        } else {
            ++expected.boot;
        }
    } else if (field == "revision") {
        if (onDevice) {
            ++peer.revision;
        } else {
            ++expected.revision;
        }
    } else if (field == "state") {
        if (onDevice) {
            peer.active = 1;
        } else {
            expected.state = 1;
        }
    } else {
        ++expected.generation;
    }
    QSignalSpy failed(&adapter, &UomDeviceAdapter::activationFailed);
    adapter.setActivation(12, expected, true);
    QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, TestTimeout::mediumMs());
    QCOMPARE(failed[0][0].toULongLong(), quint64(12));
    QVERIFY(!failed[0][2].toBool());
    QCOMPARE(peer.mutations, 0);
}

void AmovUomDeviceAdapterTest::_receiptFailure_data()
{
    QTest::addColumn<QString>("failure");
    for (const char* value : {"durability", "digest", "unknown", "readback", "revision", "reboot", "disconnect"}) {
        QTest::newRow(value) << QString::fromLatin1(value);
    }
}

void AmovUomDeviceAdapterTest::_receiptFailure()
{
    QFETCH(QString, failure);
    AmovActivationPeer peer(mockLink(), vehicle());
    AmovUomDeviceAdapter adapter(vehicle());
    adapter.refresh();
    QTRY_VERIFY_WITH_TIMEOUT(adapter.canWrite(), TestTimeout::mediumMs());
    if (failure == "durability") {
        peer.omitDurability = true;
    } else if (failure == "digest") {
        peer.wrongDigest = true;
    } else if (failure == "unknown") {
        peer.unknownReceipt = true;
    } else if (failure == "readback") {
        peer.afterMutation = [&peer]() { peer.active = 0; };
    } else if (failure == "revision") {
        peer.afterMutation = [&peer]() { ++peer.revision; };
    } else if (failure == "reboot") {
        peer.afterMutation = [&peer]() { ++peer.boot; };
    } else {
        peer.afterMutation = [this]() { emit vehicle() -> vehicleLinkManager()->communicationLostChanged(true); };
    }
    QSignalSpy failed(&adapter, &UomDeviceAdapter::activationFailed);
    QSignalSpy written(&adapter, &UomDeviceAdapter::activationWritten);
    adapter.setActivation(13, adapter.device(), true);
    QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, TestTimeout::mediumMs());
    QVERIFY(failed[0][2].toBool());
    QCOMPARE(written.count(), 0);
    QCOMPARE(peer.mutations, 1);
    QVERIFY(!adapter.canWrite());
    QCOMPARE(peer.error, QString());
}

void AmovUomDeviceAdapterTest::_multipleDevicesRequireSelection()
{
    AmovActivationPeer peer(mockLink(), vehicle());
    peer.nodes = QByteArray::fromHex("7c7d7d");
    AmovUomDeviceAdapter adapter(vehicle());
    adapter.refresh();
    QTRY_VERIFY_WITH_TIMEOUT(!adapter.busy(), TestTimeout::mediumMs());
    QCOMPARE(adapter.devices().size(), 2);
    QCOMPARE(adapter.selectedDevice(), -1);
    QVERIFY(!adapter.canWrite());
    adapter.selectDevice(1);
    QVERIFY(adapter.canWrite());
    QCOMPARE(adapter.selectedDevice(), 1);
    const auto old = adapter.device();
    adapter.selectDevice(0);
    QSignalSpy failed(&adapter, &UomDeviceAdapter::activationFailed);
    adapter.setActivation(14, old, true);
    QCOMPARE(failed.count(), 1);
    QCOMPARE(peer.mutations, 0);
}

void AmovUomDeviceAdapterTest::_concurrentRequestsAndAddressIsolation()
{
    AmovActivationPeer peer(mockLink(), vehicle());
    AmovUomDeviceAdapter adapter(vehicle());
    adapter.refresh();
    QTRY_VERIFY_WITH_TIMEOUT(adapter.canWrite(), TestTimeout::mediumMs());
    peer.hold = true;
    peer.foreignRepliesFirst = true;
    QSignalSpy read(&adapter, &UomDeviceAdapter::activationRead);
    QSignalSpy failed(&adapter, &UomDeviceAdapter::activationFailed);
    adapter.readActivation(101);
    QTRY_COMPARE_WITH_TIMEOUT(peer.held.size(), 4, TestTimeout::mediumMs());
    adapter.readActivation(102);
    QCOMPARE(failed.count(), 1);
    QCOMPARE(failed[0][0].toULongLong(), quint64(102));
    peer.release();
    QTRY_COMPARE_WITH_TIMEOUT(read.count(), 1, TestTimeout::mediumMs());
    QCOMPARE(read[0][0].toULongLong(), quint64(101));
    QVERIFY(adapter.canWrite());
}

void AmovUomDeviceAdapterTest::_connectionLossInvalidatesLateReply()
{
    AmovActivationPeer peer(mockLink(), vehicle());
    AmovUomDeviceAdapter adapter(vehicle());
    adapter.refresh();
    QTRY_VERIFY_WITH_TIMEOUT(adapter.canWrite(), TestTimeout::mediumMs());
    peer.hold = true;
    QSignalSpy read(&adapter, &UomDeviceAdapter::activationRead);
    QSignalSpy failed(&adapter, &UomDeviceAdapter::activationFailed);
    adapter.readActivation(103);
    QTRY_COMPARE_WITH_TIMEOUT(peer.held.size(), 1, TestTimeout::mediumMs());
    emit vehicle() -> vehicleLinkManager()->communicationLostChanged(true);
    QCOMPARE(failed.count(), 1);
    QVERIFY(!failed[0][2].toBool());
    peer.release();
    peer.hold = false;
    adapter.refresh();
    QTRY_VERIFY_WITH_TIMEOUT(adapter.canWrite(), TestTimeout::mediumMs());
    QCOMPARE(read.count(), 0);
}

void AmovUomDeviceAdapterTest::_responseCorrelation_data()
{
    QTest::addColumn<QString>("field");
    for (const char* value : {"session", "request", "target-system", "target-component", "node", "operation",
                              "source-system", "source-component"}) {
        QTest::newRow(value) << QString::fromLatin1(value);
    }
}

void AmovUomDeviceAdapterTest::_responseCorrelation()
{
    QFETCH(QString, field);
    AmovActivationPeer peer(mockLink(), vehicle());
    AmovUomDeviceAdapter adapter(vehicle());
    adapter.refresh();
    QTRY_VERIFY_WITH_TIMEOUT(adapter.canWrite(), TestTimeout::mediumMs());
    peer.hold = true;
    QSignalSpy read(&adapter, &UomDeviceAdapter::activationRead);
    QSignalSpy failed(&adapter, &UomDeviceAdapter::activationFailed);
    adapter.readActivation(104);
    QTRY_COMPARE_WITH_TIMEOUT(peer.held.size(), 1, TestTimeout::mediumMs());
    auto message = peer.held.first();
    mavlink_amov_device_response_t foreign{};
    mavlink_msg_amov_device_response_decode(&message, &foreign);
    int sourceSystem = vehicle()->id();
    int sourceComponent = vehicle()->defaultComponentId();
    if (field == "session") {
        ++foreign.session_id;
    } else if (field == "request") {
        ++foreign.request_id;
    } else if (field == "target-system") {
        --foreign.target_system;
    } else if (field == "target-component") {
        --foreign.target_component;
    } else if (field == "node") {
        --foreign.node_id;
    } else if (field == "operation") {
        foreign.operation = 0;
    } else if (field == "source-system") {
        ++sourceSystem;
    } else {
        ++sourceComponent;
    }
    mavlink_msg_amov_device_response_encode(sourceSystem, sourceComponent, &message, &foreign);
    // Deliver synchronously to prove the unrelated response cannot complete the pending read.
    emit vehicle() -> mavlinkMessageReceived(message);
    QCOMPARE(read.count(), 0);
    QCOMPARE(failed.count(), 0);
    QVERIFY(adapter.busy());
    peer.release();
    QTRY_COMPARE_WITH_TIMEOUT(read.count(), 1, TestTimeout::mediumMs());
    QCOMPARE(read[0][0].toULongLong(), quint64(104));
}

void AmovUomDeviceAdapterTest::_readTimeout()
{
    AmovActivationPeer peer(mockLink(), vehicle());
    AmovUomDeviceAdapter adapter(vehicle());
    adapter.refresh();
    QTRY_VERIFY_WITH_TIMEOUT(adapter.canWrite(), TestTimeout::mediumMs());
    peer.hold = true;
    QSignalSpy read(&adapter, &UomDeviceAdapter::activationRead);
    QSignalSpy failed(&adapter, &UomDeviceAdapter::activationFailed);
    adapter.readActivation(105);
    QTRY_COMPARE_WITH_TIMEOUT(peer.held.size(), 1, TestTimeout::mediumMs());
    QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, TestTimeout::longMs());
    QCOMPARE(failed[0][0].toULongLong(), quint64(105));
    QVERIFY(!failed[0][2].toBool());
    QVERIFY(!adapter.canWrite());
    emit vehicle() -> mavlinkMessageReceived(peer.held.first());
    QCOMPARE(read.count(), 0);
    QCOMPARE(failed.count(), 1);
}
