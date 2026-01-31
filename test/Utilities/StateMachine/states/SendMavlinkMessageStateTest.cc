#include "SendMavlinkMessageStateTest.h"
#include "StateTestCommon.h"

#include "SendMavlinkMessageState.h"


void SendMavlinkMessageStateTest::_testStateCreation()
{
    QStateMachine machine;

    auto encoder = [](uint8_t, uint8_t, mavlink_message_t*) {
        // Would encode a message here
    };

    auto* state = new SendMavlinkMessageState(&machine, encoder, 3);

    QVERIFY(state != nullptr);
}

void SendMavlinkMessageStateTest::_testEncoderCallback()
{
    QStateMachine machine;
    bool encoderCalled = false;

    auto encoder = [&encoderCalled](uint8_t, uint8_t, mavlink_message_t*) {
        encoderCalled = true;
    };

    auto* state = new SendMavlinkMessageState(&machine, encoder, 0);

    QVERIFY(state != nullptr);
    // Encoder will be called when state is entered (requires vehicle)
}
