#ifndef QGCMAVLINKMESSAGESENDER_H
#define QGCMAVLINKMESSAGESENDER_H

#include <QWidget>
#include <QTreeWidgetItem>
#include <QMap>
#include <QTimer>
#include "MAVLinkProtocol.h"

namespace Ui {
class QGCMAVLinkMessageSender;
}

class QGCMAVLinkMessageSender : public QWidget
{
    Q_OBJECT

    friend class QTimer;
    
public:
    explicit QGCMAVLinkMessageSender(MAVLinkProtocol* mavlink, QWidget *parent = 0);
    ~QGCMAVLinkMessageSender();
    
protected:
    mavlink_message_info_t messageInfo[256];    ///< Meta information about all messages
    MAVLinkProtocol* protocol;                  ///< MAVLink protocol
    QMap<int, float> messagesHz;                ///< Used to store update rate in Hz
    QTimer refreshTimer;
    QMap<unsigned int, QTimer*> sendTimers;
    QMap<unsigned int, QTreeWidgetItem*> managementItems;
    QMap<unsigned int, QTreeWidgetItem*> treeWidgetItems;  ///< Messages

    /** @brief Create the tree view of all messages */
    void createTreeView();
    /** @brief Create one field of one message in the tree view of all messages */
    void createField(int msgid, int fieldid, QTreeWidgetItem* item);
    /** @brief Send message with values taken from tree view */
    bool sendMessage(unsigned int id);

protected slots:
    /** @brief Read / display values in UI */
    void refresh();

private:
    Ui::QGCMAVLinkMessageSender *ui;
};

#endif // QGCMAVLINKMESSAGESENDER_H
