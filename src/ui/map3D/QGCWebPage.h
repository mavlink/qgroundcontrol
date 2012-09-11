#ifndef QGCWEBPAGE_H
#define QGCWEBPAGE_H

#include <QWebPage>

class QGCWebPage : public QWebPage
{
    Q_OBJECT
public:
    explicit QGCWebPage(QObject *parent = 0);

signals:

public slots:

protected:
    void javaScriptConsoleMessage ( const QString & message, int lineNumber, const QString & sourceID );
#ifdef Q_OS_MAC
    QString userAgentForUrl ( const QUrl & url ) const;
#endif

};

#endif // QGCWEBPAGE_H
