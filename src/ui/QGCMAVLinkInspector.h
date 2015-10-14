#ifndef QGCMAVLINKINSPECTOR_H
#define QGCMAVLINKINSPECTOR_H

#include <QMap>
#include <QTimer>

#include "QGCDockWidget.h"
#include "MAVLinkProtocol.h"
#include "Vehicle.h"

namespace Ui {
    class QGCMAVLinkInspector;
}

class QTreeWidgetItem;
class UASInterface;

class QGCMAVLinkInspector : public QGCDockWidget
{
    Q_OBJECT

public:
    explicit QGCMAVLinkInspector(const QString& title, QAction* action, MAVLinkProtocol* protocol, QWidget *parent = 0);
    ~QGCMAVLinkInspector();

public slots:
    void receiveMessage(LinkInterface* link,mavlink_message_t message);
    /** @brief Clear all messages */
    void clearView();
    /** @brief Update view */
    void refreshView();
    /** @brief Add component to the list */
    void addComponent(int uas, int component, const QString& name);
    /** @Brief Select a system through the drop down menu */
    void selectDropDownMenuSystem(int dropdownid);
    /** @Brief Select a component through the drop down menu */
    void selectDropDownMenuComponent(int dropdownid);

    void rateTreeItemChanged(QTreeWidgetItem* paramItem, int column);

protected:
    MAVLinkProtocol *_protocol;     ///< MAVLink instance
    int selectedSystemID;          ///< Currently selected system
    int selectedComponentID;       ///< Currently selected component
    QMap<int, int> systems;     ///< Already observed systems
    QMap<int, int> components; ///< Already observed components
    QMap<int, float> onboardMessageInterval; ///< Stores the onboard selected data rate
    QMap<int, QTreeWidgetItem*> rateTreeWidgetItems; ///< Available rate tree widget items
    QTimer updateTimer; ///< Only update at 1 Hz to not overload the GUI
    mavlink_message_info_t messageInfo[256]; // Store the metadata for all available MAVLink messages.

    QMap<int, QTreeWidgetItem* > uasTreeWidgetItems; ///< Tree of available uas with their widget
    QMap<int, QMap<int, QTreeWidgetItem*>* > uasMsgTreeItems; ///< Stores the widget of the received message for each UAS

    QMap<int, mavlink_message_t* > uasMessageStorage; ///< Stores the messages for every UAS

    QMap<int, QMap<int, float>* > uasMessageHz; ///< Stores the frequency of each message of each UAS
    QMap<int, QMap<int, unsigned int>* > uasMessageCount; ///< Stores the message count of each message of each UAS

    QMap<int, QMap<int, quint64>* > uasLastMessageUpdate; ///< Stores the time of the last message for each message of each UAS

    /* @brief Update one message field */
    void updateField(int sysid, int msgid, int fieldid, QTreeWidgetItem* item);
    /** @brief Rebuild the list of components */
    void rebuildComponentList();
    /** @brief Change the stream interval */
    void changeStreamInterval(int msgid, int interval);
    /* @brief Create a new tree for a new UAS */
    void addUAStoTree(int sysId);

    static const unsigned int updateInterval; ///< The update interval of the refresh function
    static const float updateHzLowpass; ///< The low-pass filter value for the frequency of each message
    
private slots:
    void _vehicleAdded(Vehicle* vehicle);

private:
    Ui::QGCMAVLinkInspector *ui;
};

#endif // QGCMAVLINKINSPECTOR_H
