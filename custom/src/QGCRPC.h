/*!
 *   @brief Typhoon H QGCCorePlugin Declaration
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#pragma once

#include <QObject>
#include <QtRemoteObjects>

#if 0
//-----------------------------------------------------------------------------
class QGCRemote : public QObject
{
    Q_OBJECT
public:
    virtual bool    IsConnected         () = 0;
    virtual void    Close               () = 0;
    virtual bool    SendMessage         (uint32_t message, QByteArray data) = 0;
signals:
    void            connected           ();
    void            ConnectionClosed    ();
    void            HandleMessage       (uint32_t message, QByteArray data);
    void            ErrorMessage        (QString message);
};
#endif
