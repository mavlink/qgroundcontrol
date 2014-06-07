#ifndef QGCUASFILEMANAGER_H
#define QGCUASFILEMANAGER_H

#include <QObject>
#include "UASInterface.h"

class QGCUASFileManager : public QObject
{
    Q_OBJECT
public:
    QGCUASFileManager(QObject* parent, UASInterface* uas);

signals:
    void statusMessage(const QString& msg);
    void resetStatusMessages();

public slots:
    void receiveMessage(LinkInterface* link, mavlink_message_t message);
    void nothingMessage();
    void listRecursively(const QString &from);
    void downloadPath(const QString& from, const QString& to);
//    void timedOut();

protected:
    struct RequestHeader
        {
            uint8_t		magic;
            uint8_t		session;
            uint8_t		opcode;
            uint8_t		size;
            uint32_t	crc32;
            uint32_t	offset;
            uint8_t		data[];
        };

    enum Opcode
        {
            kCmdNone,	// ignored, always acked
            kCmdTerminate,	// releases sessionID, closes file
            kCmdReset,	// terminates all sessions
            kCmdList,	// list files in <path> from <offset>
            kCmdOpen,	// opens <path> for reading, returns <session>
            kCmdRead,	// reads <size> bytes from <offset> in <session>
            kCmdCreate,	// creates <path> for writing, returns <session>
            kCmdWrite,	// appends <size> bytes at <offset> in <session>
            kCmdRemove,	// remove file (only if created by server?)

            kRspAck,
            kRspNak
        };

    enum ErrorCode
        {
            kErrNone,
            kErrNoRequest,
            kErrNoSession,
            kErrSequence,
            kErrNotDir,
            kErrNotFile,
            kErrEOF,
            kErrNotAppend,
            kErrTooBig,
            kErrIO,
            kErrPerm
        };


    enum OperationState
        {
            kCOIdle,    // not doing anything

            kCOList,    // waiting for a List response
        };

    OperationState _current_operation;
    unsigned _retry_counter;

    UASInterface* _mav;
    quint16 _encdata_seq;

    unsigned _session_id;       // session ID for current session
    unsigned _list_offset;      // offset for the current List operation
    QString _list_path;     // path for the current List operation

    void sendTerminate();
    void sendReset();

    void sendList();
    void listDecode(const uint8_t *data, unsigned len);

    static quint32 crc32(const uint8_t *src, unsigned len, unsigned state);

};

#endif // QGCUASFILEMANAGER_H
