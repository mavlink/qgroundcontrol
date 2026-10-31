// SPDX-License-Identifier: MIT
// Activation subset of the AMOV management payload. The flight controller forwards it without interpretation.
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace amov_device {

static constexpr uint8_t VERSION = 1;
static constexpr size_t PAYLOAD_SIZE = 192;
static constexpr size_t REQUEST_SIZE = 108;
static constexpr size_t RESPONSE_SIZE = 192;
static constexpr size_t NAME_SIZE = 16;
static constexpr size_t VALUE_SIZE = 64;
static constexpr size_t UID_SIZE = 6;
static constexpr size_t SN_SIZE = 20;

enum class Operation : uint8_t
{
    INFO = 1,
    STATUS = 5,
    ACTIVATE = 6
};
enum class Type : uint8_t
{
    NONE = 0,
    UINT8 = 1,
    UINT32 = 2,
    FLOAT = 3,
    CHAR20 = 4,
    CHAR64 = 5,
    INT8 = 6
};
enum class Result : uint8_t
{
    OK = 0,
    ACCEPTED = 1,
    SAVED = 2,
    BUSY = 3,
    NOT_FOUND = 4,
    READ_ONLY = 5,
    LOCKED = 6,
    INVALID = 7,
    STORAGE_ERROR = 8,
    CONFLICT = 9,
    UNSUPPORTED = 10,
    UNKNOWN = 11
};

enum Flag : uint16_t
{
    WRITABLE = 1,
    MASKED = 2,
    REBOOT_REQUIRED = 4,
    ACTION = 8,
    UNUSED_IN_PROFILE = 16,
    DEFAULT_AVAILABLE = 32,
    DURABLE = 64,
    LEGACY_IDENTITY = 128
};

struct Request
{
    Operation operation = Operation::INFO;
    Type type = Type::NONE;
    uint32_t transaction = 0;
    uint32_t boot = 0;
    uint8_t uid[UID_SIZE] = {};
    uint16_t index = 0;
    uint32_t revision = 0;
    uint8_t value_length = 0;
    char name[NAME_SIZE + 1] = {};
    uint8_t value[VALUE_SIZE] = {};
};

struct Response
{
    Operation operation = Operation::INFO;
    Result result = Result::OK;
    Type type = Type::NONE;
    uint32_t transaction = 0;
    uint32_t boot = 0;
    uint8_t uid[UID_SIZE] = {};
    uint16_t index = 0;
    uint16_t total = 0;
    uint16_t flags = 0;
    uint8_t value_length = 0;
    uint8_t max_length = 0;
    uint8_t min_length = 0;
    uint8_t activation_state = 0;
    uint32_t revision = 0;
    char name[NAME_SIZE + 1] = {};
    uint8_t value[VALUE_SIZE] = {};
    float minimum = 0;
    float maximum = 0;
    uint8_t default_length = 0;
    uint8_t default_value[VALUE_SIZE] = {};
};

inline uint16_t read_u16(const uint8_t* data)
{
    return uint16_t(data[0]) | (uint16_t(data[1]) << 8);
}

inline uint32_t read_u32(const uint8_t* data)
{
    return uint32_t(data[0]) | (uint32_t(data[1]) << 8) | (uint32_t(data[2]) << 16) | (uint32_t(data[3]) << 24);
}

inline void write_u16(uint8_t* data, uint16_t value)
{
    data[0] = uint8_t(value);
    data[1] = uint8_t(value >> 8);
}

inline void write_u32(uint8_t* data, uint32_t value)
{
    for (unsigned i = 0; i < 4; ++i) {
        data[i] = uint8_t(value >> (8 * i));
    }
}

inline float read_float(const uint8_t* data)
{
    const uint32_t bits = read_u32(data);
    float value;
    static_assert(sizeof(value) == sizeof(bits), "Management protocol requires 32-bit float");
    memcpy(&value, &bits, sizeof(value));
    return value;
}

inline void write_float(uint8_t* data, float value)
{
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    write_u32(data, bits);
}

inline bool valid_operation(Operation operation)
{
    return operation == Operation::INFO || operation == Operation::STATUS || operation == Operation::ACTIVATE;
}

inline bool valid_type(Type type)
{
    return uint8_t(type) <= uint8_t(Type::INT8);
}

inline bool valid_name(const char* name)
{
    bool ended = false;
    for (size_t i = 0; i < NAME_SIZE; ++i) {
        if (name[i] == '\0') {
            ended = true;
        } else if (ended || name[i] < 0x21 || name[i] > 0x7e) {
            return false;
        }
    }
    return name[NAME_SIZE] == '\0';
}

// Canonical operation fields prevent two peers from silently interpreting an overloaded field differently.
// Target identity, permissions, ranges and persistence remain the device handler's responsibility.
inline bool valid_request_shape(const Request& request)
{
    if (!valid_operation(request.operation) || !valid_type(request.type) || !valid_name(request.name)) {
        return false;
    }
    if (request.operation == Operation::ACTIVATE) {
        return request.type == Type::NONE && !request.name[0] && !request.index && request.value_length == 21 &&
               request.value[0] <= 1;
    }
    if (request.revision || request.index || request.type != Type::NONE || request.name[0]) {
        return false;
    }
    return request.operation == Operation::STATUS ? request.value_length == 32 : request.value_length == 0;
}

inline bool encode_request(const Request& request, uint8_t* data, size_t capacity)
{
    if (!data || capacity < REQUEST_SIZE || !valid_operation(request.operation) || !valid_type(request.type) ||
        !request.transaction || request.value_length > VALUE_SIZE || !valid_request_shape(request)) {
        return false;
    }
    memset(data, 0, REQUEST_SIZE);
    data[0] = VERSION;
    data[1] = uint8_t(request.operation);
    data[2] = uint8_t(request.type);
    write_u32(data + 4, request.transaction);
    write_u32(data + 8, request.boot);
    memcpy(data + 12, request.uid, UID_SIZE);
    write_u16(data + 18, request.index);
    write_u32(data + 20, request.revision);
    data[24] = request.value_length;
    memcpy(data + 28, request.name, NAME_SIZE);
    memcpy(data + 44, request.value, request.value_length);
    return true;
}

inline bool decode_request(const uint8_t* data, size_t length, Request& request)
{
    if (!data || length != REQUEST_SIZE || data[0] != VERSION || data[3] || data[25] || data[26] || data[27] ||
        !valid_operation(Operation(data[1])) || !valid_type(Type(data[2])) || !read_u32(data + 4) ||
        data[24] > VALUE_SIZE) {
        return false;
    }
    Request decoded;
    decoded.operation = Operation(data[1]);
    decoded.type = Type(data[2]);
    decoded.transaction = read_u32(data + 4);
    decoded.boot = read_u32(data + 8);
    memcpy(decoded.uid, data + 12, UID_SIZE);
    decoded.index = read_u16(data + 18);
    decoded.revision = read_u32(data + 20);
    decoded.value_length = data[24];
    memcpy(decoded.name, data + 28, NAME_SIZE);
    memcpy(decoded.value, data + 44, decoded.value_length);
    if (!valid_request_shape(decoded)) {
        return false;
    }
    for (size_t i = decoded.value_length; i < VALUE_SIZE; ++i) {
        if (data[44 + i]) {
            return false;
        }
    }
    request = decoded;
    return true;
}

inline bool encode_response(const Response& response, uint8_t* data, size_t capacity)
{
    if (!data || capacity < RESPONSE_SIZE || !valid_operation(response.operation) || !valid_type(response.type) ||
        uint8_t(response.result) > uint8_t(Result::UNKNOWN) || !response.transaction ||
        response.value_length > VALUE_SIZE || response.default_length > VALUE_SIZE ||
        response.max_length > VALUE_SIZE || response.min_length > response.max_length ||
        response.activation_state > 2 || (response.flags & ~uint16_t(255)) || !valid_name(response.name)) {
        return false;
    }
    memset(data, 0, RESPONSE_SIZE);
    data[0] = VERSION;
    data[1] = uint8_t(response.operation);
    data[2] = uint8_t(response.result);
    data[3] = uint8_t(response.type);
    write_u32(data + 4, response.transaction);
    write_u32(data + 8, response.boot);
    memcpy(data + 12, response.uid, UID_SIZE);
    write_u16(data + 18, response.index);
    write_u16(data + 20, response.total);
    write_u16(data + 22, response.flags);
    data[24] = response.value_length;
    data[25] = response.max_length;
    data[26] = response.min_length;
    data[27] = response.activation_state;
    write_u32(data + 28, response.revision);
    memcpy(data + 32, response.name, NAME_SIZE);
    memcpy(data + 48, response.value, response.value_length);
    write_float(data + 112, response.minimum);
    write_float(data + 116, response.maximum);
    data[120] = response.default_length;
    memcpy(data + 121, response.default_value, response.default_length);
    return true;
}

inline bool decode_response(const uint8_t* data, size_t length, Response& response)
{
    if (!data || length != RESPONSE_SIZE || data[0] != VERSION || !valid_operation(Operation(data[1])) ||
        data[2] > uint8_t(Result::UNKNOWN) || !valid_type(Type(data[3])) || !read_u32(data + 4) ||
        (read_u16(data + 22) & ~uint16_t(255)) || data[24] > VALUE_SIZE || data[25] > VALUE_SIZE ||
        data[26] > data[25] || data[27] > 2 || data[120] > VALUE_SIZE) {
        return false;
    }
    for (size_t i = 185; i < RESPONSE_SIZE; ++i) {
        if (data[i]) {
            return false;
        }
    }
    for (size_t i = data[24]; i < VALUE_SIZE; ++i) {
        if (data[48 + i]) {
            return false;
        }
    }
    for (size_t i = data[120]; i < VALUE_SIZE; ++i) {
        if (data[121 + i]) {
            return false;
        }
    }
    Response decoded;
    decoded.operation = Operation(data[1]);
    decoded.result = Result(data[2]);
    decoded.type = Type(data[3]);
    decoded.transaction = read_u32(data + 4);
    decoded.boot = read_u32(data + 8);
    memcpy(decoded.uid, data + 12, UID_SIZE);
    decoded.index = read_u16(data + 18);
    decoded.total = read_u16(data + 20);
    decoded.flags = read_u16(data + 22);
    decoded.value_length = data[24];
    decoded.max_length = data[25];
    decoded.min_length = data[26];
    decoded.activation_state = data[27];
    decoded.revision = read_u32(data + 28);
    memcpy(decoded.name, data + 32, NAME_SIZE);
    memcpy(decoded.value, data + 48, decoded.value_length);
    decoded.minimum = read_float(data + 112);
    decoded.maximum = read_float(data + 116);
    decoded.default_length = data[120];
    memcpy(decoded.default_value, data + 121, decoded.default_length);
    if (!valid_name(decoded.name)) {
        return false;
    }
    response = decoded;
    return true;
}

}  // namespace amov_device
