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
    MainWindow::QGC_MAINWINDOW_STYLE style = MainWindow::instance()->getStyle();
    ui->styleChooser->setCurrentIndex(style);
    connect(ui->styleChooser, SIGNAL(currentIndexChanged(int)), this, SLOT(styleChanged(int)));
    connect(ui->styleCustomButton, SIGNAL(clicked()), this, SLOT(selectStylesheet()));
    connect(ui->styleDefaultButton, SIGNAL(clicked()), this, SLOT(setDefaultStyle()));

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
    QString oldStylesheet(ui->styleSheetFile->text());
    QFile styleSheet(oldStylesheet);
    if (styleSheet.exists() && oldStylesheet[0] != ':')
    {
        findDir = styleSheet.fileName();
    }
    else
    {
        findDir = QDir::homePath();
    }

    // Prompt the user to select a new style sheet. Do nothing if they cancel.
    QString newStyleFileName = QFileDialog::getOpenFileName(this, tr("Specify stylesheet"), findDir, tr("CSS Stylesheet (*.css);;"));
    if (newStyleFileName.isNull()) {
        return;
    }

    // Load the new style sheet if a valid one was selected, notifying the user
    // of an error if necessary.
    QFile newStyleFile(newStyleFileName);
    if (!newStyleFile.exists() || !updateStyle(newStyleFileName))
    {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setText(tr("QGroundControl did not load a new style"));
        msgBox.setInformativeText(tr("Stylesheet file %1 was not readable").arg(newStyleFileName));
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();
    }
    // And update the UI as needed.
    else
    {
        switch (ui->styleChooser->currentIndex())
        {
        case 0:
            darkStyleSheet = newStyleFileName;
            ui->styleSheetFile->setText(darkStyleSheet);
            MainWindow::instance()->loadStyle(MainWindow::QGC_MAINWINDOW_STYLE_DARK, darkStyleSheet);
        case 1:
            lightStyleSheet = newStyleFileName;
            ui->styleSheetFile->setText(lightStyleSheet);
            MainWindow::instance()->loadStyle(MainWindow::QGC_MAINWINDOW_STYLE_LIGHT, lightStyleSheet);
        }
    }
}

bool QGCSettingsWidget::updateStyle(QString style)
{
    switch (ui->styleChooser->currentIndex())
    {
    case 0:
        darkStyleSheet = style;
        return MainWindow::instance()->loadStyle(MainWindow::QGC_MAINWINDOW_STYLE_DARK, darkStyleSheet);
    case 1:
        lightStyleSheet = style;
        return MainWindow::instance()->loadStyle(MainWindow::QGC_MAINWINDOW_STYLE_LIGHT, lightStyleSheet);
    default:
        return false;
    }
}

void QGCSettingsWidget::styleChanged(int index)
{
    if (index == 1)
    {
        ui->styleSheetFile->setText(lightStyleSheet);
        updateStyle(lightStyleSheet);
    }
    else
    {
        ui->styleSheetFile->setText(darkStyleSheet);
        updateStyle(darkStyleSheet);
    }
}

void QGCSettingsWidget::setDefaultStyle()
{
    if (ui->styleChooser->currentIndex() == 1)
    {
        lightStyleSheet = MainWindow::defaultLightStyle;
        ui->styleSheetFile->setText(lightStyleSheet);
        updateStyle(lightStyleSheet);
    }
    else
    {
        darkStyleSheet = MainWindow::defaultDarkStyle;
        ui->styleSheetFile->setText(darkStyleSheet);
        updateStyle(darkStyleSheet);
    }
}
