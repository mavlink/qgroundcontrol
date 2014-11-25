#ifndef MAVLINKSETTINGSWIDGET_H
#define MAVLINKSETTINGSWIDGET_H

#include <QWidget>

#include "MAVLinkProtocol.h"

namespace Ui
{
class MAVLinkSettingsWidget;
}

class MAVLinkSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    MAVLinkSettingsWidget(MAVLinkProtocol* protocol, QWidget *parent = 0);
    ~MAVLinkSettingsWidget();

public slots:
    /** @brief Enable DroneOS forwarding */
    void enableDroneOS(bool enable);

    void setDroneOSKey(QString key);

    void setDroneOSHost(QString host);

protected:
    MAVLinkProtocol* protocol;
    void changeEvent(QEvent *e);
    void hideEvent(QHideEvent* event);

private:
    Ui::MAVLinkSettingsWidget *m_ui;
};

#endif // MAVLINKSETTINGSWIDGET_H
