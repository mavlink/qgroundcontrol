/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include <QSettings>
#include <QDesktopWidget>

#include "SettingsDialog.h"
#include "MainWindow.h"
#include "ui_SettingsDialog.h"

#include "LinkManager.h"
#include "MAVLinkProtocol.h"
#include "MAVLinkSettingsWidget.h"
#include "GAudioOutput.h"
#include "QGCApplication.h"
#include "QGCFileDialog.h"
#include "QGCMessageBox.h"
#include "MainToolBarController.h"
#include "FlightMapSettings.h"

SettingsDialog::SettingsDialog(QWidget *parent, Qt::WindowFlags flags)
    : QDialog(parent, flags)
    , _ui(new Ui::SettingsDialog)
{
    _ui->setupUi(this);

    // Center the window on the screen.
    QDesktopWidget *desktop = QApplication::desktop();
    int screen = desktop->screenNumber(parent);

    QRect position = frameGeometry();
    position.moveCenter(QApplication::desktop()->availableGeometry(screen).center());
    move(position.topLeft());

    MAVLinkSettingsWidget* pMavsettings  = new MAVLinkSettingsWidget(qgcApp()->toolbox()->mavlinkProtocol(), this);

    // Add the MAVLink settings pane
    _ui->tabWidget->addTab(pMavsettings,  "MAVLink");

    this->window()->setWindowTitle(tr("QGroundControl Settings"));

    _ui->tabWidget->setCurrentWidget(pMavsettings);

    connect(_ui->buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
}

SettingsDialog::~SettingsDialog()
{
    delete _ui;
}
