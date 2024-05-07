#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QCoreApplication>
#include <QtCore/QLoggingCategory>

#include "GeoTagWorker.h"

Q_DECLARE_LOGGING_CATEGORY(ULogParserLog)

class ULogParser
{
    Q_DECLARE_TR_FUNCTIONS(ULogParser)

public:
    ULogParser() = default;
    ~ULogParser() = default;

    /// @return true: failed, errorMessage set
    bool getTagsFromLog(QByteArray& log, QList<GeoTagWorker::cameraFeedbackPacket>& cameraFeedback, QString& errorMessage);

private:

    QMap<QString, int> _cameraCaptureOffsets; // <fieldName, fieldOffset>
    int _cameraCaptureMsgID;

    const char _ULogMagic[8] = {'U', 'L', 'o', 'g', 0x01, 0x12, 0x35};

    static int sizeOfType(QString& typeName);
    int sizeOfFullType(QString &typeNameFull);
    QString extractArraySize(QString& typeNameFull, int& arraySize);

    bool parseFieldFormat(QString& fields);

    enum class ULogMessageType : uint8_t {
        FORMAT = 'F',
        DATA = 'D',
        INFO = 'I',
        PARAMETER = 'P',
        ADD_LOGGED_MSG = 'A',
        REMOVE_LOGGED_MSG = 'R',
        SYNC = 'S',
        DROPOUT = 'O',
        LOGGING = 'L',
    };

    #define ULOG_MSG_HEADER_LEN 3
    struct ULogMessageHeader {
        uint16_t msgSize;
        uint8_t msgType;
    };

    struct ULogMessageFormat {
        uint16_t msgSize;
        uint8_t msgType;

        char format[2096];
    };

    struct ULogMessageAddLogged {
	  uint16_t msgSize;
      uint8_t msgType;

	  uint8_t multiID;
	  uint16_t msgID;
	  char msgName[255];
	};
};
