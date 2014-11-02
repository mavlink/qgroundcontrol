/**
 * @file MessageWindow.cpp
 * @brief MessageWindow Implementation.
 * @see MessageWindow.h
 * @author Micha? Policht
 */

#include <stdio.h>
#include "MessageWindow.h"
#include <QMessageBox>
#include <QCoreApplication>
#include <QMutexLocker>

const char *MessageWindow::WINDOW_TITLE = "Message Window";
MessageWindow *MessageWindow::MsgHandler = NULL;

MessageWindow::MessageWindow(QWidget *parent, Qt::WindowFlags flags)
    : QDockWidget(parent, flags),
      msgTextEdit(this)
{
    setWindowTitle(tr(WINDOW_TITLE));
    msgTextEdit.setReadOnly(true);
    setWidget(&msgTextEdit);

    MessageWindow::MsgHandler = this;
}

//static
QString MessageWindow::QtMsgToQString(QtMsgType type, const char *msg)
{
    switch (type) {
    case QtDebugMsg:
        return QLatin1String("Debug: ")+QLatin1String(msg);
    case QtWarningMsg:
        return QLatin1String("Warning: ")+QLatin1String(msg);
    case QtCriticalMsg:
        return QLatin1String("Critical: ")+QLatin1String(msg);
    case QtFatalMsg:
        return QLatin1String("Fatal: ")+QLatin1String(msg);
    default:
        return QLatin1String("Unrecognized message type: ")+QLatin1String(msg);
    }
}

//static
void MessageWindow::AppendMsgWrapper(QtMsgType type, const char *msg)
{
    static QMutex mutex;
    QMutexLocker locker(&mutex);

    if (MessageWindow::MsgHandler != NULL)
        return MessageWindow::MsgHandler->postMsgEvent(type, msg);
    else
        fprintf(stderr, "%s", MessageWindow::QtMsgToQString(type, msg).toLatin1().data());
}

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
void MessageWindow::AppendMsgWrapper(QtMsgType type, const QMessageLogContext & /*context*/, const QString &msg)
{
    AppendMsgWrapper(type, msg.toLatin1().data());
}
#endif

void MessageWindow::customEvent(QEvent *event)
{
    if (static_cast<MessageWindow::EventType>(event->type()) == MessageWindow::MessageEventType)
        msgTextEdit.append(dynamic_cast<MessageEvent *>(event)->msg);
}

void MessageWindow::postMsgEvent(QtMsgType type, const char *msg)
{
    QString qmsg = MessageWindow::QtMsgToQString(type, msg);
    switch (type) {
    case QtDebugMsg:
        break;
    case QtWarningMsg:
        qmsg.prepend(QLatin1String("<FONT color=\"#FF0000\">"));
        qmsg.append(QLatin1String("</FONT>"));
        break;
    case QtCriticalMsg:
        if (QMessageBox::critical(this, QLatin1String("Critical Error"), qmsg,
                                  QMessageBox::Ignore,
                                  QMessageBox::Abort,
                                  QMessageBox::NoButton) == QMessageBox::Abort)
            abort(); // core dump
        qmsg.prepend(QLatin1String("<B><FONT color=\"#FF0000\">"));
        qmsg.append(QLatin1String("</FONT></B>"));
        break;
    case QtFatalMsg:
        QMessageBox::critical(this, QLatin1String("Fatal Error"), qmsg, QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton);
        abort(); // deliberately core dump
    }
    //it's impossible to change GUI directly from thread other than the main thread
    //so post message encapsulated by MessageEvent to the main thread's event queue
    QCoreApplication::postEvent(this, new MessageEvent(qmsg));
}

MessageEvent::MessageEvent(QString &msg):
    QEvent(static_cast<QEvent::Type>(MessageWindow::MessageEventType))
{
    this->msg = msg;
}
