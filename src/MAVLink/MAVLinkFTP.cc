#include "MAVLinkFTP.h"

#include <QtCore/QByteArray>
#include <QtCore/QRegularExpression>
#include <QtCore/QStringDecoder>
#include <QtCore/QtEndian>
#include <cstring>
#include <limits>

bool MavlinkFTP::setRequestData(Request& request, QStringView value)
{
    if (value.contains(QChar::Null)) {
        request.hdr.size = 0;
        return false;
    }

    const QByteArray encodedValue = value.toString().toUtf8();
    if (encodedValue.size() > dataCapacity) {
        request.hdr.size = 0;
        return false;
    }
    if (!encodedValue.isEmpty()) {
        std::memcpy(request.data, encodedValue.constData(), static_cast<size_t>(encodedValue.size()));
    }
    request.hdr.size = static_cast<uint8_t>(encodedValue.size());
    return true;
}

std::optional<uint32_t> MavlinkFTP::decodeOpenFileLength(const Request& response)
{
    if (response.hdr.size != sizeof(uint32_t)) {
        return std::nullopt;
    }
    return qFromLittleEndian<uint32_t>(response.data);
}

void MavlinkFTP::setOpenFileLength(Request& response, uint32_t length)
{
    qToLittleEndian<uint32_t>(length, response.data);
    response.hdr.size = sizeof(length);
}

MavlinkFTP::ResponseValidation MavlinkFTP::validateResponse(const Request& response,
                                                            MAV_FTP_OPCODE expectedRequestOpcode)
{
    if (static_cast<MAV_FTP_OPCODE>(response.hdr.req_opcode) != expectedRequestOpcode) {
        return {ResponseValidationResult::Unrelated, ResponseValidationError::None};
    }

    const auto responseOpcode = static_cast<MAV_FTP_OPCODE>(response.hdr.opcode);
    if ((responseOpcode != MAV_FTP_OPCODE_ACK) && (responseOpcode != MAV_FTP_OPCODE_NAK)) {
        return {ResponseValidationResult::Malformed, ResponseValidationError::InvalidResponseOpcode};
    }
    if (response.hdr.size > sizeof(response.data)) {
        return {ResponseValidationResult::Malformed, ResponseValidationError::OversizedPayload};
    }
    if (responseOpcode == MAV_FTP_OPCODE_NAK) {
        if (response.hdr.size == 0) {
            return {ResponseValidationResult::Malformed, ResponseValidationError::MissingNakErrorCode};
        }
        if (!decodeNak(response).has_value()) {
            return {ResponseValidationResult::Malformed, ResponseValidationError::InvalidNakPayload};
        }
    }

    return {};
}

std::optional<MavlinkFTP::NakError> MavlinkFTP::decodeNak(const Request& response)
{
    if ((response.hdr.size < 1) || (response.hdr.size > sizeof(response.data))) {
        return std::nullopt;
    }

    const auto errorCode = static_cast<MAV_FTP_ERR>(response.data[0]);
    const uint8_t expectedSize = (errorCode == MAV_FTP_ERR_FAILERRNO) ? 2 : 1;
    if (response.hdr.size != expectedSize) {
        return std::nullopt;
    }

    NakError result{errorCode, std::nullopt};
    if (errorCode == MAV_FTP_ERR_FAILERRNO) {
        result.errorNumber = response.data[1];
    }
    return result;
}

MavlinkFTP::DirectoryEntryParseResult MavlinkFTP::parseDirectoryEntry(QStringView record)
{
    DirectoryEntryParseResult result;
    if (record.isEmpty()) {
        result.error = DirectoryEntryParseError::Empty;
        return result;
    }

    switch (record.front().unicode()) {
        case 'F':
            result.entry.type = DirectoryEntryType::File;
            break;
        case 'D':
            result.entry.type = DirectoryEntryType::Directory;
            break;
        case 'S':
            result.entry.type = DirectoryEntryType::Skip;
            break;
        default:
            result.error = DirectoryEntryParseError::UnknownType;
            return result;
    }

    const QStringView fields = record.sliced(1);
    const qsizetype firstTab = fields.indexOf(QLatin1Char('\t'));
    const QStringView name = firstTab < 0 ? fields : fields.first(firstTab);
    result.entry.name = name.toString();

    if (result.entry.type == DirectoryEntryType::Skip) {
        return result;
    }
    if (name.isEmpty()) {
        result.error = DirectoryEntryParseError::MissingName;
        return result;
    }
    if (firstTab < 0) {
        if (result.entry.type == DirectoryEntryType::File) {
            result.error = DirectoryEntryParseError::MissingSize;
        }
        return result;
    }

    const QStringView metadata = fields.sliced(firstTab + 1);
    const qsizetype secondTab = metadata.indexOf(QLatin1Char('\t'));
    const QStringView sizeField = secondTab < 0 ? metadata : metadata.first(secondTab);
    bool sizeIsValid = false;
    const quint64 size = sizeField.toULongLong(&sizeIsValid);
    if (!sizeIsValid) {
        result.error = DirectoryEntryParseError::InvalidSize;
        return result;
    }
    result.entry.size = size;

    if (secondTab < 0) {
        return result;
    }

    const QStringView trailingFields = metadata.sliced(secondTab + 1);
    const qsizetype thirdTab = trailingFields.indexOf(QLatin1Char('\t'));
    const QStringView modificationTimeField = thirdTab < 0 ? trailingFields : trailingFields.first(thirdTab);

    bool modificationTimeIsValid = false;
    const qint64 modificationTime = modificationTimeField.toLongLong(&modificationTimeIsValid);
    if (!modificationTimeIsValid) {
        result.error = DirectoryEntryParseError::InvalidModificationTime;
        return result;
    }
    result.entry.modificationTime = modificationTime;
    if (thirdTab >= 0) {
        result.error = DirectoryEntryParseError::ExtraFields;
    }
    return result;
}

QString MavlinkFTP::formatDirectoryEntry(const DirectoryEntry& entry, bool includeModificationTime)
{
    if ((entry.type != DirectoryEntryType::Skip) && entry.name.isEmpty()) {
        return QString();
    }
    if ((entry.type == DirectoryEntryType::File) && !entry.size.has_value()) {
        return QString();
    }

    QChar type;
    switch (entry.type) {
        case DirectoryEntryType::File:
            type = QLatin1Char('F');
            break;
        case DirectoryEntryType::Directory:
            type = QLatin1Char('D');
            break;
        case DirectoryEntryType::Skip:
            type = QLatin1Char('S');
            break;
        case DirectoryEntryType::Unknown:
            return QString();
    }

    QString result(type);
    result += entry.name;
    if (entry.size.has_value()) {
        result += QLatin1Char('\t') + QString::number(*entry.size);
        if (includeModificationTime) {
            result += QLatin1Char('\t') + QString::number(entry.modificationTime.value_or(0));
        }
    }
    return result;
}

MavlinkFTP::DirectoryPayloadParseResult MavlinkFTP::parseDirectoryPayload(const Request& response)
{
    DirectoryPayloadParseResult result;
    if (response.hdr.size == 0) {
        result.error = DirectoryPayloadParseError::Empty;
        return result;
    }
    if (response.hdr.size > sizeof(response.data)) {
        result.error = DirectoryPayloadParseError::Oversized;
        return result;
    }

    const char* current = reinterpret_cast<const char*>(response.data);
    const char* const end = current + response.hdr.size;
    while (current < end) {
        const char* const terminator =
            static_cast<const char*>(std::memchr(current, '\0', static_cast<size_t>(end - current)));
        if (!terminator) {
            result.records.clear();
            result.error = DirectoryPayloadParseError::UnterminatedEntry;
            return result;
        }
        if (terminator == current) {
            result.records.clear();
            result.error = DirectoryPayloadParseError::EmptyEntry;
            return result;
        }

        QStringDecoder decoder(QStringDecoder::Utf8);
        const QString record = decoder(QByteArrayView(current, terminator - current));
        if (decoder.hasError()) {
            result.records.clear();
            result.error = DirectoryPayloadParseError::InvalidUtf8;
            return result;
        }
        result.records.append(record);
        current = terminator + 1;
    }
    return result;
}

bool MavlinkFTP::isMavlinkFtpUri(QStringView uri)
{
    const QString prefix = QStringLiteral("%1://").arg(QLatin1StringView(uriScheme));
    return uri.startsWith(prefix, Qt::CaseInsensitive);
}

MavlinkFTP::UriParseResult MavlinkFTP::parseUri(uint8_t defaultComponentId, QStringView uri)
{
    UriParseResult result;
    result.componentId =
        (defaultComponentId == MAV_COMP_ID_ALL) ? static_cast<uint8_t>(MAV_COMP_ID_AUTOPILOT1) : defaultComponentId;
    result.path = uri.toString();

    if (uri.contains(QChar::Null)) {
        result.error = UriParseError::EmbeddedNull;
        return result;
    }

    const bool hasMavlinkFtpScheme = isMavlinkFtpUri(uri);
    if (hasMavlinkFtpScheme) {
        const qsizetype prefixSize = QStringLiteral("%1://").arg(QLatin1StringView(uriScheme)).size();
        result.path.remove(0, prefixSize);
    } else if (result.path.contains(QStringLiteral("://"))) {
        result.error = UriParseError::InvalidScheme;
        return result;
    }

    static const QRegularExpression componentExpression(QStringLiteral("^/?\\[;comp=(\\d+)\\]"));
    const QRegularExpressionMatch componentMatch = componentExpression.match(result.path);
    if (componentMatch.hasMatch()) {
        bool componentIdIsValid = false;
        const uint componentId = componentMatch.capturedView(1).toUInt(&componentIdIsValid);
        if (!componentIdIsValid || (componentId > (std::numeric_limits<uint8_t>::max)())) {
            result.error = UriParseError::InvalidComponentId;
            return result;
        }
        result.componentId = static_cast<uint8_t>(componentId);

        const bool hadLeadingSlash = result.path.startsWith(QLatin1Char('/'));
        result.path.remove(0, componentMatch.capturedLength());
        if (hadLeadingSlash && !result.path.startsWith(QLatin1Char('/'))) {
            result.path.prepend(QLatin1Char('/'));
        }
    } else {
        QStringView selectorCandidate(result.path);
        if (selectorCandidate.startsWith(QLatin1Char('/'))) {
            selectorCandidate = selectorCandidate.sliced(1);
        }
        if (selectorCandidate.startsWith(QLatin1StringView("[;comp="))) {
            result.error = UriParseError::InvalidComponentId;
            return result;
        }
    }

    if (hasMavlinkFtpScheme && !result.path.startsWith(QLatin1Char('/'))) {
        result.path.prepend(QLatin1Char('/'));
    }
    if (result.path.toUtf8().size() > dataCapacity) {
        result.error = UriParseError::PathTooLong;
    }
    return result;
}

QString MavlinkFTP::opCodeToString(MAV_FTP_OPCODE opcode)
{
    switch (opcode) {
        case MAV_FTP_OPCODE_NONE:
            return QStringLiteral("None");
        case MAV_FTP_OPCODE_TERMINATESESSION:
            return QStringLiteral("Terminate Session");
        case MAV_FTP_OPCODE_RESETSESSION:
            return QStringLiteral("Reset Sessions");
        case MAV_FTP_OPCODE_LISTDIRECTORY:
            return QStringLiteral("List Directory");
        case MAV_FTP_OPCODE_OPENFILERO:
            return QStringLiteral("Open File RO");
        case MAV_FTP_OPCODE_READFILE:
            return QStringLiteral("Read File");
        case MAV_FTP_OPCODE_CREATEFILE:
            return QStringLiteral("Create File");
        case MAV_FTP_OPCODE_WRITEFILE:
            return QStringLiteral("Write File");
        case MAV_FTP_OPCODE_REMOVEFILE:
            return QStringLiteral("Remove File");
        case MAV_FTP_OPCODE_CREATEDIRECTORY:
            return QStringLiteral("Create Directory");
        case MAV_FTP_OPCODE_REMOVEDIRECTORY:
            return QStringLiteral("Remove Directory");
        case MAV_FTP_OPCODE_OPENFILEWO:
            return QStringLiteral("Open File WO");
        case MAV_FTP_OPCODE_TRUNCATEFILE:
            return QStringLiteral("Truncate File");
        case MAV_FTP_OPCODE_RENAME:
            return QStringLiteral("Rename");
        case MAV_FTP_OPCODE_CALCFILECRC:
            return QStringLiteral("Calc File CRC32");
        case MAV_FTP_OPCODE_BURSTREADFILE:
            return QStringLiteral("Burst Read File");
        case MAV_FTP_OPCODE_LISTDIRECTORYWITHTIME:
            return QStringLiteral("List Directory With Time");
        case MAV_FTP_OPCODE_ACK:
            return QStringLiteral("Ack");
        case MAV_FTP_OPCODE_NAK:
            return QStringLiteral("Nak");
        default:
            return QStringLiteral("Unknown OpCode");
    }
}

QString MavlinkFTP::errorCodeToString(MAV_FTP_ERR errorCode)
{
    switch (errorCode) {
        case MAV_FTP_ERR_NONE:
            return QStringLiteral("None");
        case MAV_FTP_ERR_FAIL:
            return QStringLiteral("Fail");
        case MAV_FTP_ERR_FAILERRNO:
            return QStringLiteral("Fail Errno");
        case MAV_FTP_ERR_INVALIDDATASIZE:
            return QStringLiteral("Invalid Data Size");
        case MAV_FTP_ERR_INVALIDSESSION:
            return QStringLiteral("Invalid Session");
        case MAV_FTP_ERR_NOSESSIONSAVAILABLE:
            return QStringLiteral("No Sessions Available");
        case MAV_FTP_ERR_EOF:
            return QStringLiteral("EOF");
        case MAV_FTP_ERR_UNKNOWNCOMMAND:
            return QStringLiteral("Unknown Command");
        case MAV_FTP_ERR_FILEEXISTS:
            return QStringLiteral("File Already Exists");
        case MAV_FTP_ERR_FILEPROTECTED:
            return QStringLiteral("File Protected");
        case MAV_FTP_ERR_FILENOTFOUND:
            return QStringLiteral("File Not Found");
        default:
            return QStringLiteral("Unknown Error");
    }
}
