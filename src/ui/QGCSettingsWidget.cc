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
    ui(new Ui::QGCSettingsWidget),
    mainWindow((MainWindow*)parent)
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
    ui->reconnectCheckBox->setChecked(mainWindow->autoReconnectEnabled());
    connect(ui->reconnectCheckBox, SIGNAL(clicked(bool)), mainWindow, SLOT(enableAutoReconnect(bool)));

    // Low power mode
    ui->lowPowerCheckBox->setChecked(mainWindow->lowPowerModeEnabled());
    connect(ui->lowPowerCheckBox, SIGNAL(clicked(bool)), mainWindow, SLOT(enableLowPowerMode(bool)));

    //Dock widget title bars
    ui->titleBarCheckBox->setChecked(mainWindow->dockWidgetTitleBarsEnabled());
    connect(ui->titleBarCheckBox,SIGNAL(clicked(bool)),mainWindow,SLOT(enableDockWidgetTitleBars(bool)));

    // Intialize the style UI to the proper values obtained from the MainWindow.
    MainWindow::QGC_MAINWINDOW_STYLE style = mainWindow->getStyle();
    ui->styleChooser->setCurrentIndex(style);
    if (style == MainWindow::QGC_MAINWINDOW_STYLE_DARK)
    {
        ui->styleSheetFile->setText(mainWindow->getDarkStyleSheet());
    }
    else
    {
        ui->styleSheetFile->setText(mainWindow->getLightStyleSheet());
    }

    // And then connect all the signals for the UI for changing styles.
    connect(ui->styleChooser, SIGNAL(currentIndexChanged(int)), this, SLOT(styleChanged(int)));
    connect(ui->styleCustomButton, SIGNAL(clicked()), this, SLOT(selectStylesheet()));
    connect(ui->styleDefaultButton, SIGNAL(clicked()), this, SLOT(setDefaultStyle()));
    connect(ui->styleSheetFile, SIGNAL(editingFinished()), this, SLOT(lineEditFinished()));

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
        ui->styleSheetFile->setText(newStyleFileName);
    }
}

bool QGCSettingsWidget::updateStyle(QString style)
{
    switch (ui->styleChooser->currentIndex())
    {
    case 0:
        return mainWindow->loadStyle(MainWindow::QGC_MAINWINDOW_STYLE_DARK, style);
    case 1:
        return mainWindow->loadStyle(MainWindow::QGC_MAINWINDOW_STYLE_LIGHT, style);
    default:
        return false;
    }
}

void QGCSettingsWidget::lineEditFinished()
{
    QString newStyleFileName(ui->styleSheetFile->text());
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
}

void QGCSettingsWidget::styleChanged(int index)
{
    if (index == 1)
    {
        ui->styleSheetFile->setText(mainWindow->getLightStyleSheet());
        mainWindow->loadStyle(MainWindow::QGC_MAINWINDOW_STYLE_LIGHT, mainWindow->getLightStyleSheet());
    }
    else
    {
        ui->styleSheetFile->setText(mainWindow->getDarkStyleSheet());
        mainWindow->loadStyle(MainWindow::QGC_MAINWINDOW_STYLE_DARK, mainWindow->getDarkStyleSheet());
    }
}

void QGCSettingsWidget::setDefaultStyle()
{
    if (ui->styleChooser->currentIndex() == 1)
    {
        ui->styleSheetFile->setText(MainWindow::defaultLightStyle);
        mainWindow->loadStyle(MainWindow::QGC_MAINWINDOW_STYLE_LIGHT, MainWindow::defaultLightStyle);
    }
    else
    {
        ui->styleSheetFile->setText(MainWindow::defaultDarkStyle);
        mainWindow->loadStyle(MainWindow::QGC_MAINWINDOW_STYLE_DARK, MainWindow::defaultDarkStyle);
    }
}
