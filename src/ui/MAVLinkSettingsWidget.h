#ifndef MAVLINKSETTINGSWIDGET_H
#define MAVLINKSETTINGSWIDGET_H

#include <QtGui/QWidget>

#include "MAVLinkProtocol.h"

namespace Ui {
    class MAVLinkSettingsWidget;
}

class MAVLinkSettingsWidget : public QWidget {
    Q_OBJECT
public:
    MAVLinkSettingsWidget(MAVLinkProtocol* protocol, QWidget *parent = 0);
    ~MAVLinkSettingsWidget();

protected:
    MAVLinkProtocol* protocol;
    void changeEvent(QEvent *e);

private:
    Ui::MAVLinkSettingsWidget *m_ui;
};

#endif // MAVLINKSETTINGSWIDGET_H
