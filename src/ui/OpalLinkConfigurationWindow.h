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

public slots:

    void allowSettingsAccess(bool enabled);

private:
    Ui::OpalLinkSettings ui;
    OpalLink* link;
};

#endif // OPALLINKCONFIGURATIONWINDOW_H
