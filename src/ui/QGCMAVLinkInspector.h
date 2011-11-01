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
    mavlink_message_t receivedMessages[256]; ///< Available / known messages
    QMap<int, QTreeWidgetItem*> treeWidgetItems;   ///< Available tree widget items
    QTimer updateTimer; ///< Only update at 1 Hz to not overload the GUI
    mavlink_message_info_t messageInfo[256];

    // Update one message field
    void updateField(int msgid, int fieldid, QTreeWidgetItem* item);

private:
    Ui::QGCMAVLinkInspector *ui;
};

#endif // QGCMAVLINKINSPECTOR_H
