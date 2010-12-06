#include "QGCWebPage.h"
#include <QDebug>

QGCWebPage::QGCWebPage(QObject *parent) :
    QWebPage(parent)
{
}

void QGCWebPage::javaScriptConsoleMessage ( const QString & message, int lineNumber, const QString & sourceID )
{
    qDebug() << "JAVASCRIPT: " << lineNumber << sourceID << message;
}
