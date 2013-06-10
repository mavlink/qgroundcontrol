#include "HUD2IndicatorMessage.h"
#include "HUD2Math.h"
#include "QGCMAVLink.h"

HUD2IndicatorMessage::HUD2IndicatorMessage(QWidget *parent) :
    QWidget(parent)
{
    labelFont.setPixelSize(15);
}

void HUD2IndicatorMessage::updateTextMessage(int uasid, int componentid,
                                             int severity, QString text)
{
    hud2_msg_t msg;

    msg.timeout = QTime::currentTime();
    msg.timeout = msg.timeout.addSecs(4);

    switch(severity){
    case MAV_SEVERITY_WARNING:
        msg.color = Qt::yellow;
        msg.timeout = msg.timeout.addSecs(4);
        break;

    case MAV_SEVERITY_NOTICE:
        msg.color = Qt::green;
        break;

    case MAV_SEVERITY_INFO:
        msg.color = Qt::cyan;
        break;

    case MAV_SEVERITY_DEBUG:
        msg.color = Qt::white;
        break;

    default:
        msg.color = QColor(255, 32, 32);
        msg.timeout = msg.timeout.addSecs(8);
        break;
    }

    msg.txt = QStaticText(QString::number(uasid) + ":" +
                          QString::number(componentid) + " " + text);
    list.append(msg);
}

void HUD2IndicatorMessage::updateGeometry(const QSize &size){

    qreal fontsize_percent = 2.0; // font size of labels on ribbon
    int fntsize = percent2pix_d(size, fontsize_percent);
    fntsize = qBound(7, fntsize, 50);
    labelFont.setPixelSize(fntsize);
}

void HUD2IndicatorMessage::paint(QPainter *painter){
    int N;
    hud2_msg_t msg;
    QSizeF msg_size;

    N = list.length();

    if (0 == N)
        return; // nothing to do

    painter->save();
    painter->translate(0, painter->window().height());

    while (N--){
        msg = list.at(N);
        msg_size = msg.txt.size();

        painter->translate(0, -msg_size.height());

        QRectF bgrect = QRectF(QPointF(0, 0), msg_size);

        painter->fillRect(bgrect, Qt::black);
        painter->setPen(msg.color);
        painter->setFont(labelFont);
        painter->drawStaticText(0, 0, msg.txt);

        if (msg.timeout < QTime::currentTime())
            list.removeAt(N);
    }

    painter->restore();
}
