// SPDX-License-Identifier: MIT
#include <cassert>
#include <cstring>

#include "management_protocol.h"

int main()
{
    amov_device::Request request;
    request.operation = amov_device::Operation::ACTIVATE;
    request.transaction = 0x12345678;
    request.boot = 0x90abcdef;
    request.revision = 17;
    std::memcpy(request.uid, "ABCDEF", 6);
    request.value_length = 21;
    request.value[0] = 1;
    std::memcpy(request.value + 1, "TESTAIRCRAFT00000001", 20);
    uint8_t encoded[amov_device::REQUEST_SIZE]{};
    assert(amov_device::encode_request(request, encoded, sizeof(encoded)));
    assert(encoded[1] == 6 && encoded[4] == 0x78 && encoded[7] == 0x12 && encoded[24] == 21);
    amov_device::Request decoded;
    assert(amov_device::decode_request(encoded, sizeof(encoded), decoded));
    assert(decoded.revision == 17 && decoded.boot == request.boot);
    assert(!std::memcmp(decoded.value, request.value, 21));
    encoded[107] = 1;
    assert(!amov_device::decode_request(encoded, sizeof(encoded), decoded));
    encoded[107] = 0;
    encoded[1] = 4;  // Parameter mutation is outside this activation-only codec.
    assert(!amov_device::decode_request(encoded, sizeof(encoded), decoded));
    request.operation = amov_device::Operation::STATUS;
    request.revision = 0;
    request.value_length = 32;
    assert(amov_device::encode_request(request, encoded, sizeof(encoded)));
    request.value_length = 31;
    assert(!amov_device::encode_request(request, encoded, sizeof(encoded)));
    amov_device::Response response;
    response.operation = amov_device::Operation::ACTIVATE;
    response.transaction = 1;
    response.boot = 7;
    response.revision = 18;
    response.activation_state = 1;
    response.flags = amov_device::DURABLE;
    response.result = amov_device::Result::SAVED;
    uint8_t reply[amov_device::RESPONSE_SIZE]{};
    assert(amov_device::encode_response(response, reply, sizeof(reply)));
    amov_device::Response parsed;
    assert(amov_device::decode_response(reply, sizeof(reply), parsed));
    assert(parsed.flags == amov_device::DURABLE && parsed.revision == 18);
    reply[191] = 1;
    assert(!amov_device::decode_response(reply, sizeof(reply), parsed));
    reply[191] = 0;
    reply[27] = 3;
    assert(!amov_device::decode_response(reply, sizeof(reply), parsed));
    return 0;
}
