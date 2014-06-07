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

protected:
    UASInterface* _mav;
    quint16 _encdata_seq;

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

    static quint32 crc32(const uint8_t *src, unsigned len, unsigned state);

};

#endif // QGCUASFILEMANAGER_H
