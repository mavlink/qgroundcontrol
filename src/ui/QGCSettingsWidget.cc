#include <QSettings>

#include "QGCSettingsWidget.h"
#include "MainWindow.h"
#include "ui_QGCSettingsWidget.h"

#include "LinkManager.h"
#include "MAVLinkProtocol.h"
#include "MAVLinkSettingsWidget.h"
#include "GAudioOutput.h"

//, Qt::WindowFlags flags

QGCSettingsWidget::QGCSettingsWidget(QWidget *parent, Qt::WindowFlags flags) :
    QDialog(parent, flags),
    ui(new Ui::QGCSettingsWidget)
{
    ui->setupUi(this);

	// Set the frame holding the options for the custom style frame to hidden by default
	ui->customStyleFrame->setVisible(false);

    // Add all protocols
    QList<ProtocolInterface*> protocols = LinkManager::instance()->getProtocols();
    foreach (ProtocolInterface* protocol, protocols) {
        MAVLinkProtocol* mavlink = dynamic_cast<MAVLinkProtocol*>(protocol);
        if (mavlink) {
            MAVLinkSettingsWidget* msettings = new MAVLinkSettingsWidget(mavlink, this);
            ui->tabWidget->addTab(msettings, "MAVLink");
        }
    }

    this->window()->setWindowTitle(tr("QGroundControl Settings"));

    // Audio preferences
    ui->audioMuteCheckBox->setChecked(GAudioOutput::instance()->isMuted());
    connect(ui->audioMuteCheckBox, SIGNAL(toggled(bool)), GAudioOutput::instance(), SLOT(mute(bool)));
    connect(GAudioOutput::instance(), SIGNAL(mutedChanged(bool)), ui->audioMuteCheckBox, SLOT(setChecked(bool)));

    // Reconnect
    ui->reconnectCheckBox->setChecked(MainWindow::instance()->autoReconnectEnabled());
    connect(ui->reconnectCheckBox, SIGNAL(clicked(bool)), MainWindow::instance(), SLOT(enableAutoReconnect(bool)));

    // Low power mode
    ui->lowPowerCheckBox->setChecked(MainWindow::instance()->lowPowerModeEnabled());
    connect(ui->lowPowerCheckBox, SIGNAL(clicked(bool)), MainWindow::instance(), SLOT(enableLowPowerMode(bool)));

    //Dock widget title bars
    ui->titleBarCheckBox->setChecked(MainWindow::instance()->dockWidgetTitleBarsEnabled());
    connect(ui->titleBarCheckBox,SIGNAL(clicked(bool)),MainWindow::instance(),SLOT(enableDockWidgetTitleBars(bool)));

    // Style
    MainWindow::QGC_MAINWINDOW_STYLE style = (MainWindow::QGC_MAINWINDOW_STYLE)MainWindow::instance()->getStyle();
    ui->styleChooser->setCurrentIndex(style);
	connect(ui->styleChooser, SIGNAL(currentIndexChanged(int)), this, SLOT(styleChanged(int)));
	connect(ui->customStyleFileButton, SIGNAL(clicked()), this, SLOT(selectStylesheet()));

    // Close / destroy
    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(deleteLater()));
}

QGCSettingsWidget::~QGCSettingsWidget()
{
    delete ui;
}

void QGCSettingsWidget::selectStylesheet()
{
    // Let user select style sheet. The root directory for the file picker is the user's home directory if they haven't loaded a custom style.
	// Otherwise it defaults to the directory of that custom file.
	QString findDir;
	if (MainWindow::instance()->getStyle() == MainWindow::QGC_MAINWINDOW_STYLE_CUSTOM_DARK || MainWindow::instance()->getStyle() == MainWindow::QGC_MAINWINDOW_STYLE_CUSTOM_LIGHT)
	{
		findDir = QDir::homePath();
	}
	else
	{
		findDir = MainWindow::instance()->getStyleSheet();
	}

	QString newStyleFileName = QFileDialog::getOpenFileName(this, tr("Specify stylesheet"), findDir, tr("CSS Stylesheet (*.css);;"));

    // Load the new style sheet if a valid one was selected.
    if (!newStyleFileName.isNull())
    {
        QFile styleSheet(newStyleFileName);
        if (styleSheet.exists())
        {
            if (!updateStyle())
			{
				QMessageBox msgBox;
				msgBox.setIcon(QMessageBox::Information);
				msgBox.setText(tr("QGroundControl did not load a new style"));
				msgBox.setInformativeText(tr("Stylesheet file %1 was not readable").arg(newStyleFileName));
				msgBox.setStandardButtons(QMessageBox::Ok);
				msgBox.setDefaultButton(QMessageBox::Ok);
				msgBox.exec();
			}
        }
		else
		{
			QMessageBox msgBox;
			msgBox.setIcon(QMessageBox::Information);
			msgBox.setText(tr("QGroundControl did not load a new style"));
			msgBox.setInformativeText(tr("Stylesheet file %1 was not readable").arg(newStyleFileName));
			msgBox.setStandardButtons(QMessageBox::Ok);
			msgBox.setDefaultButton(QMessageBox::Ok);
			msgBox.exec();
		}
    }
}

bool QGCSettingsWidget::updateStyle()
{
	switch (ui->styleChooser->currentIndex())
	{
	case 0:
		return MainWindow::instance()->loadStyle(MainWindow::QGC_MAINWINDOW_STYLE_DARK, QString());
	case 1:
		return MainWindow::instance()->loadStyle(MainWindow::QGC_MAINWINDOW_STYLE_LIGHT, QString());
	case 2:
		return MainWindow::instance()->loadStyle(MainWindow::QGC_MAINWINDOW_STYLE_CUSTOM_DARK, QString());
	case 3:
		return MainWindow::instance()->loadStyle(MainWindow::QGC_MAINWINDOW_STYLE_CUSTOM_LIGHT, QString());
	default:
		return false;
	}
}

void QGCSettingsWidget::styleChanged(int index)
{
	// If a custom style is selected, enable the advanced view for the custom stylesheet. Otherwise,
	// make sure it's hidden.
	if (index == 2 || index == 3)
	{
		ui->customStyleFrame->setVisible(true);
	}
	else
	{
		ui->customStyleFrame->setVisible(false);
	}
	
	// And trigger a style update.
	updateStyle();
}