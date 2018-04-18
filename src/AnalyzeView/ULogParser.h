#ifndef ULOGPARSER_H
#define ULOGPARSER_H

#include <QGeoCoordinate>
#include <QDebug>
#include <QCoreApplication>

#include "GeoTagController.h"

#define ULOG_FILE_HEADER_LEN 16

class ULogParser
{
    Q_DECLARE_TR_FUNCTIONS(ULogParser)

public:
    ULogParser();
    ~ULogParser();

    /// @return true: failed, errorMessage set
    bool getTagsFromLog(QByteArray& log, QList<GeoTagWorker::cameraFeedbackPacket>& cameraFeedback, QString& errorMessage);

private:

    QMap<QString, int> _cameraCaptureOffsets; // <fieldName, fieldOffset>
    int _cameraCaptureMsgID;

    const char _ULogMagic[8] = {'U', 'L', 'o', 'g', 0x01, 0x12, 0x35};

    int sizeOfType(QString& typeName);
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

#endif // ULOGPARSER_H
