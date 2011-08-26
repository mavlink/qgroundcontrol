#ifndef QGCMAVLINKINSPECTOR_H
#define QGCMAVLINKINSPECTOR_H

#include <QWidget>
#include <QMap>
#include <QTimer>

#include "MAVLinkProtocol.h"

namespace Ui {
    class QGCMAVLinkInspector;
}

class QTreeWidgetItem;

class QGCMAVLinkInspector : public QWidget
{
    Q_OBJECT

public:
    explicit QGCMAVLinkInspector(MAVLinkProtocol* protocol, QWidget *parent = 0);
    ~QGCMAVLinkInspector();

public slots:
    void receiveMessage(LinkInterface* link,mavlink_message_t message);
    void refreshView();

protected:
    QMap<int, quint64> lastFieldUpdate; ///< Used to switch between highlight and non-highlighting color
    QMap<int, mavlink_message_t> receivedMessages; ///< Available / known messages
    QMap<int, QTreeWidgetItem*> treeWidgetItems;   ///< Available tree widget items
    QTimer updateTimer; ///< Only update at 1 Hz to not overload the GUI

private:
    Ui::QGCMAVLinkInspector *ui;
};

#endif // QGCMAVLINKINSPECTOR_H
