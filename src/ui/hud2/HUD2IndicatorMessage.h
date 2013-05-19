#ifndef HUD2INDICATORMESSAGE_H
#define HUD2INDICATORMESSAGE_H

#include <QtGui>
#include <QWidget>

typedef struct my_msg_t{
    QColor color;
    QStaticText txt;
    QTime timeout;
} my_msg_t;

class HUD2IndicatorMessage : public QWidget
{
    Q_OBJECT
public:
    explicit HUD2IndicatorMessage(QWidget *parent);
    void paint(QPainter *painter);

public slots:
    void updateTextMessage(int uasid, int componentid, int severity, QString text);
    void updateGeometry(const QSize &size);

signals:

public:
    int timeout;
    int maxmsg;
    QFont labelFont;
    QList<my_msg_t> list;
};

#endif // HUD2INDICATORMESSAGE_H
