#include "QGCViewModeSelection.h"
#include "ui_QGCViewModeSelection.h"
#include "QGC.h"
#include "MainWindow.h"

QGCViewModeSelection::QGCViewModeSelection(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QGCViewModeSelection),
    selected(false)
{
    ui->setupUi(this);

    connect(ui->viewModeGeneric, SIGNAL(clicked()), this, SLOT(selectGeneric()));
    connect(ui->viewModeAR, SIGNAL(clicked()), this, SLOT(selectWifi()));
    connect(ui->viewModePX4, SIGNAL(clicked()), this, SLOT(selectPX4()));
    connect(ui->notAgainCheckBox, SIGNAL(clicked(bool)), this, SIGNAL(settingsStorageRequested(bool)));
}

QGCViewModeSelection::~QGCViewModeSelection()
{
    delete ui;
}

enum MainWindow::CUSTOM_MODE QGCViewModeSelection::waitForInput() {
    while (!selected)
        QGC::SLEEP::msleep(200);

    return mode;
}

void QGCViewModeSelection::selectGeneric() {
    emit customViewModeSelected(MainWindow::CUSTOM_MODE_NONE);
    mode = MainWindow::CUSTOM_MODE_NONE;
    selected = true;
}

void QGCViewModeSelection::selectWifi() {
    emit customViewModeSelected(MainWindow::CUSTOM_MODE_WIFI);
    mode = MainWindow::CUSTOM_MODE_WIFI;
    selected = true;
}

void QGCViewModeSelection::selectPX4() {
    emit customViewModeSelected(MainWindow::CUSTOM_MODE_PX4);
    mode = MainWindow::CUSTOM_MODE_PX4;
    selected = true;
}
