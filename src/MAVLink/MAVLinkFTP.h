#pragma once

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QStringView>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "MAVLinkLib.h"

/// Stateless encoding and validation helpers for the MAVLink FTP wire protocol.
class MavlinkFTP
{
public:
    /// This is the fixed length portion of the protocol data.
    /// This needs to be packed, because it is typecast from mavlink_file_transfer_protocol_t.payload, which starts
    /// at a 3-byte offset, causing unaligned access to seqNumber and offset.
    MAVPACKED(typedef struct RequestHeader {
        uint16_t seqNumber;     ///< Sequence number for message.
        uint8_t session;        ///< Session id for read and write commands.
        uint8_t opcode;         ///< MAV_FTP_OPCODE command or response opcode.
        uint8_t size;           ///< Size of data.
        uint8_t req_opcode;     ///< Request opcode returned in ACK and NAK messages.
        uint8_t burstComplete;  ///< Whether the current burst is complete.
        uint8_t padding;        ///< 32-bit alignment padding.
        uint32_t offset;        ///< Offset for list and read commands.
    })
    RequestHeader;

    MAVPACKED(typedef struct Request {
        RequestHeader hdr;
        uint8_t data[sizeof(((mavlink_file_transfer_protocol_t*) nullptr)->payload) - sizeof(RequestHeader)];
    })
    Request;

    static_assert(sizeof(RequestHeader) == 12, "MAVLink FTP request header wire layout changed");
    static_assert(alignof(RequestHeader) == 1, "MAVLink FTP request header must remain packed");
    static_assert(offsetof(RequestHeader, offset) == 8, "MAVLink FTP request offset field moved");
    static_assert(sizeof(Request) == sizeof(((mavlink_file_transfer_protocol_t*) nullptr)->payload),
                  "MAVLink FTP request no longer fills the MAVLink payload");
    static_assert(alignof(Request) == 1, "MAVLink FTP request must remain packed");
    static_assert(offsetof(Request, data) == sizeof(RequestHeader), "MAVLink FTP request data field moved");

    enum class DirectoryEntryType : uint8_t
    {
        Unknown,
        File,
        Directory,
        Skip,
    };

    enum class DirectoryEntryParseError : uint8_t
    {
        None,
        Empty,
        UnknownType,
        MissingName,
        MissingSize,
        InvalidSize,
        InvalidModificationTime,
        ExtraFields,
    };

    struct DirectoryEntry
    {
        DirectoryEntryType type = DirectoryEntryType::Unknown;
        QString name;
        std::optional<quint64> size;
        std::optional<qint64> modificationTime;
    };

    struct DirectoryEntryParseResult
    {
        DirectoryEntry entry;
        DirectoryEntryParseError error = DirectoryEntryParseError::None;

        bool valid() const { return error == DirectoryEntryParseError::None; }
    };

    enum class DirectoryPayloadParseError : uint8_t
    {
        None,
        Empty,
        EmptyEntry,
        Oversized,
        UnterminatedEntry,
        InvalidUtf8,
    };

    struct DirectoryPayloadParseResult
    {
        QStringList records;
        DirectoryPayloadParseError error = DirectoryPayloadParseError::None;

        bool valid() const { return error == DirectoryPayloadParseError::None; }
    };

    enum class ResponseValidationResult : uint8_t
    {
        Valid,
        Unrelated,
        Malformed,
    };

    enum class ResponseValidationError : uint8_t
    {
        None,
        InvalidResponseOpcode,
        OversizedPayload,
        MissingNakErrorCode,
        InvalidNakPayload,
    };

    struct ResponseValidation
    {
        ResponseValidationResult result = ResponseValidationResult::Valid;
        ResponseValidationError error = ResponseValidationError::None;
    };

    struct NakError
    {
        MAV_FTP_ERR code = MAV_FTP_ERR_NONE;
        std::optional<uint8_t> errorNumber;
    };

    enum class UriParseError : uint8_t
    {
        None,
        InvalidScheme,
        InvalidComponentId,
        EmbeddedNull,
        PathTooLong,
    };

    struct UriParseResult
    {
        QString path;
        uint8_t componentId = MAV_COMP_ID_AUTOPILOT1;
        UriParseError error = UriParseError::None;

        bool valid() const { return error == UriParseError::None; }
    };

    static constexpr const char* uriScheme = "mftp";
    static constexpr qsizetype dataCapacity = sizeof(((Request*) nullptr)->data);

    static bool setRequestData(Request& request, QStringView value);
    static std::optional<uint32_t> decodeOpenFileLength(const Request& response);
    static void setOpenFileLength(Request& response, uint32_t length);
    static ResponseValidation validateResponse(const Request& response, MAV_FTP_OPCODE expectedRequestOpcode);
    static std::optional<NakError> decodeNak(const Request& response);

    static DirectoryEntryParseResult parseDirectoryEntry(QStringView record);
    static QString formatDirectoryEntry(const DirectoryEntry& entry, bool includeModificationTime = false);
    static DirectoryPayloadParseResult parseDirectoryPayload(const Request& response);

    static bool isMavlinkFtpUri(QStringView uri);
    static UriParseResult parseUri(uint8_t defaultComponentId, QStringView uri);

    static QString opCodeToString(MAV_FTP_OPCODE opcode);
    static QString errorCodeToString(MAV_FTP_ERR errorCode);
};
