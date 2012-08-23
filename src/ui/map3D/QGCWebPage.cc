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

#ifdef Q_OS_MAC64
QString QGCWebPage::userAgentForUrl ( const QUrl & url ) const
{
    Q_UNUSED(url);
    return "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_6_0) AppleWebKit/534.57.2 (KHTML, like Gecko) Version/5.1.7 Safari";
}
#endif
