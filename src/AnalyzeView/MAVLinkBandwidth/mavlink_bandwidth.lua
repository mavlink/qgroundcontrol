-- QGroundControl end-to-end MAVLink bandwidth test endpoint for ArduPilot.
-- Install as APM/scripts/mavlink_bandwidth.lua and restart scripting.

local MESSAGE_ID = 385 -- TUNNEL
local TUNNEL_PAYLOAD_TYPE = 0
local PROTOCOL_VERSION = 1
local MAGIC = "QGBW"

local OP_HELLO = 1
local OP_HELLO_ACK = 2
local OP_START = 3
local OP_DATA = 4
local OP_REPORT = 5
local OP_STOP = 6
local OP_ABORT = 7

local DIRECTION_QGC_TO_VEHICLE = 1
local DIRECTION_VEHICLE_TO_QGC = 2

local DATA_BYTES = 90
local DATA_PATTERN = string.rep(string.char(0x5A), DATA_BYTES)
local PADDING_PATTERN = string.rep(string.char(0xA5), DATA_BYTES)
local INNER_FORMAT = "<c4BBBBI4I4I4I4I4I4I4I2c90"
local OUTER_FORMAT = "<I2BBBc128"
local UPDATE_INTERVAL_MS = 10
local REPORT_INTERVAL_MS = 500
local MAX_RX_PER_UPDATE = 25
local MAX_TX_PER_UPDATE = 12
local MAX_RATE_BYTES_PER_SECOND = 250000
local MAX_DURATION_MS = 60000

local state = {
    active = false,
    channel = 0,
    target_system = 0,
    target_component = 0,
    direction = DIRECTION_QGC_TO_VEHICLE,
    session_id = 0,
    rate_bytes_per_second = 0,
    duration_ms = 0,
    start_ms = 0,
    last_report_ms = 0,
    next_tx_sequence = 0,
    last_rx_sequence = nil,
    tx_packets = 0,
    rx_packets = 0,
    lost_packets = 0,
    send_failures = 0,
    payload_bytes = 0,
}

local function now_ms()
    return millis():toint() & 0xFFFFFFFF
end

local function elapsed_ms(start_ms)
    return (now_ms() - start_ms) & 0xFFFFFFFF
end

local function reset_test(packet)
    state.active = true
    state.direction = packet.direction
    state.session_id = packet.session_id
    state.rate_bytes_per_second = math.min(packet.value1, MAX_RATE_BYTES_PER_SECOND)
    state.duration_ms = math.min(packet.value2, MAX_DURATION_MS)
    state.start_ms = now_ms()
    state.last_report_ms = state.start_ms
    state.next_tx_sequence = 0
    state.last_rx_sequence = nil
    state.tx_packets = 0
    state.rx_packets = 0
    state.lost_packets = 0
    state.send_failures = 0
    state.payload_bytes = 0
end

local function encode_inner(opcode, direction, session_id, sequence, value1, value2, value3, value4, value5,
                            data_length, data)
    return string.pack(INNER_FORMAT,
        MAGIC,
        PROTOCOL_VERSION,
        opcode,
        direction,
        0,
        session_id,
        sequence,
        value1,
        value2,
        value3,
        value4,
        value5,
        data_length,
        data)
end

local function send_inner(opcode, direction, session_id, sequence, value1, value2, value3, value4, value5,
                          data_length, data)
    local inner = encode_inner(opcode, direction, session_id, sequence, value1, value2, value3, value4, value5,
        data_length, data or PADDING_PATTERN)
    local outer = string.pack(
        OUTER_FORMAT, TUNNEL_PAYLOAD_TYPE, state.target_system, state.target_component, #inner, inner)
    return mavlink:send_chan(state.channel, MESSAGE_ID, outer) == true
end

local function send_report()
    return send_inner(
        OP_REPORT,
        state.direction,
        state.session_id,
        state.tx_packets,
        state.rx_packets,
        state.lost_packets,
        state.send_failures,
        state.payload_bytes,
        elapsed_ms(state.start_ms),
        0,
        PADDING_PATTERN)
end

local function send_abort()
    send_inner(
        OP_ABORT,
        state.direction,
        state.session_id,
        0,
        0,
        0,
        state.send_failures,
        state.payload_bytes,
        elapsed_ms(state.start_ms),
        0,
        PADDING_PATTERN)
    state.active = false
end

local function decode_message(message, channel)
    if #message < 145 then
        return nil
    end

    local payload_type, _, _, payload_length = string.unpack("<I2BBB", message, 13)
    if payload_type ~= TUNNEL_PAYLOAD_TYPE or payload_length ~= 128 then
        return nil
    end

    local payload = message:sub(18, 145)
    local magic, version, opcode, direction, flags, session_id, sequence,
        value1, value2, value3, value4, value5, data_length, data = string.unpack(INNER_FORMAT, payload)

    if magic ~= MAGIC or version ~= PROTOCOL_VERSION or data_length > DATA_BYTES then
        return nil
    end
    if direction ~= DIRECTION_QGC_TO_VEHICLE and direction ~= DIRECTION_VEHICLE_TO_QGC then
        return nil
    end

    return {
        channel = channel,
        source_system = message:byte(8),
        source_component = message:byte(9),
        opcode = opcode,
        direction = direction,
        flags = flags,
        session_id = session_id,
        sequence = sequence,
        value1 = value1,
        value2 = value2,
        value3 = value3,
        value4 = value4,
        value5 = value5,
        data_length = data_length,
        data = data,
    }
end

local function receive_data(packet)
    if not state.active or packet.session_id ~= state.session_id or
        state.direction ~= DIRECTION_QGC_TO_VEHICLE or packet.direction ~= DIRECTION_QGC_TO_VEHICLE then
        return
    end

    if state.last_rx_sequence == nil then
        state.lost_packets = state.lost_packets + packet.sequence
    else
        local expected = (state.last_rx_sequence + 1) & 0xFFFFFFFF
        local delta = (packet.sequence - expected) & 0xFFFFFFFF
        if delta > 0 and delta < 0x80000000 then
            state.lost_packets = state.lost_packets + delta
        end
    end

    state.last_rx_sequence = packet.sequence
    state.rx_packets = state.rx_packets + 1
    state.payload_bytes = state.payload_bytes + packet.data_length
end

local function handle_packet(packet)
    state.channel = packet.channel
    state.target_system = packet.source_system
    state.target_component = packet.source_component

    if packet.opcode == OP_HELLO then
        send_inner(OP_HELLO_ACK, packet.direction, packet.session_id, 0, PROTOCOL_VERSION, 0, 0, 0, 0,
            0, PADDING_PATTERN)
        return
    end

    if packet.opcode == OP_START then
        if arming:is_armed() or packet.value2 == 0 or packet.value3 ~= DATA_BYTES then
            state.session_id = packet.session_id
            state.direction = packet.direction
            send_abort()
            return
        end
        reset_test(packet)
        send_report()
    elseif packet.opcode == OP_DATA then
        receive_data(packet)
    elseif packet.opcode == OP_STOP and packet.session_id == state.session_id then
        send_report()
        state.active = false
    elseif packet.opcode == OP_ABORT and packet.session_id == state.session_id then
        state.active = false
    end
end

local function send_data(elapsed)
    if state.direction ~= DIRECTION_VEHICLE_TO_QGC then
        return
    end

    local target_payload_bytes = math.floor((elapsed * state.rate_bytes_per_second) / 1000)
    local sent_this_update = 0
    while state.payload_bytes + DATA_BYTES <= target_payload_bytes and sent_this_update < MAX_TX_PER_UPDATE do
        local sent = send_inner(
            OP_DATA,
            DIRECTION_VEHICLE_TO_QGC,
            state.session_id,
            state.next_tx_sequence,
            0,
            0,
            0,
            0,
            0,
            DATA_BYTES,
            DATA_PATTERN)
        if not sent then
            state.send_failures = state.send_failures + 1
            break
        end

        state.next_tx_sequence = (state.next_tx_sequence + 1) & 0xFFFFFFFF
        state.tx_packets = state.tx_packets + 1
        state.payload_bytes = state.payload_bytes + DATA_BYTES
        sent_this_update = sent_this_update + 1
    end
end

local function update()
    for _ = 1, MAX_RX_PER_UPDATE do
        local message, channel = mavlink:receive_chan()
        if not message then
            break
        end

        local packet = decode_message(message, channel)
        if packet then
            handle_packet(packet)
        end
    end

    if state.active then
        if arming:is_armed() then
            send_abort()
        else
            local elapsed = elapsed_ms(state.start_ms)
            if elapsed >= state.duration_ms then
                send_report()
                send_inner(OP_STOP, state.direction, state.session_id, 0, 0, 0, 0, 0, elapsed,
                    0, PADDING_PATTERN)
                state.active = false
            else
                send_data(elapsed)
                if elapsed_ms(state.last_report_ms) >= REPORT_INTERVAL_MS then
                    send_report()
                    state.last_report_ms = now_ms()
                end
            end
        end
    end

    return update, UPDATE_INTERVAL_MS
end

mavlink:init(25, 8)
mavlink:register_rx_msgid(MESSAGE_ID)

return update()
