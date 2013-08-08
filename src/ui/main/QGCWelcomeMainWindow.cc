#include "QGCWelcomeMainWindow.h"
#include "ui_QGCWelcomeMainWindow.h"
#include "MainWindow.h"
#include "QGCViewModeSelection.h"

QGCWelcomeMainWindow::QGCWelcomeMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::QGCWelcomeMainWindow),
    storeSettings(false)
{
    ui->setupUi(this);
    statusBar()->hide();

    QString windowname = qApp->applicationName() + " " + qApp->applicationVersion();
    setWindowTitle(windowname);

    viewModeSelection = new QGCViewModeSelection(this);

    connect(viewModeSelection, SIGNAL(customViewModeSelected(MainWindow::CUSTOM_MODE)), this, SIGNAL(customViewModeSelected(MainWindow::CUSTOM_MODE)));
    connect(viewModeSelection, SIGNAL(settingsStorageRequested(bool)), this, SIGNAL(settingsStorageRequested(bool)));
    connect(viewModeSelection, SIGNAL(settingsStorageRequested(bool)), this, SLOT(setStoreSettings(bool)));

    setCentralWidget(viewModeSelection);

    // Load the new stylesheet.
    QFile styleSheet(":files/styles/style-dark.css");

    // Attempt to open the stylesheet.
    if (styleSheet.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        // Signal to the user that the app will pause to apply a new stylesheet
        qApp->setOverrideCursor(Qt::WaitCursor);

        qApp->setStyleSheet(styleSheet.readAll());

        // Finally restore the cursor before returning.
        qApp->restoreOverrideCursor();
    }

    resize(780, 400);
    show();

}

QGCWelcomeMainWindow::~QGCWelcomeMainWindow()
{
    delete ui;
    delete viewModeSelection;
}
