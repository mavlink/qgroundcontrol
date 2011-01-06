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

};

#endif // QGCWEBPAGE_H
