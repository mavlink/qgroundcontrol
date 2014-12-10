#ifndef OPALLINKCONFIGURATIONWINDOW_H
#define OPALLINKCONFIGURATIONWINDOW_H

#include <QWidget>
#include <QDebug>

#include "LinkInterface.h"
#include "ui_OpalLinkSettings.h"
#include "OpalLink.h"

class OpalLinkConfigurationWindow : public QWidget
{
    Q_OBJECT
public:
    explicit OpalLinkConfigurationWindow(OpalLink* link, QWidget *parent = 0, Qt::WindowFlags flags = Qt::Sheet);
signals:

private slots:
    void _linkConnected(void);
    void _linkDisconnected(void);

private:
    void _allowSettingsAccess(bool enabled);
    
    Ui::OpalLinkSettings ui;
    OpalLink* link;
};

#endif // OPALLINKCONFIGURATIONWINDOW_H
