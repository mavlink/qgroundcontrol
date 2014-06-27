#include "QGCUASFileView.h"
#include "uas/QGCUASFileManager.h"
#include "ui_QGCUASFileView.h"

#include <QFileDialog>
#include <QDir>

QGCUASFileView::QGCUASFileView(QWidget *parent, QGCUASFileManager *manager) :
    QWidget(parent),
    _manager(manager),
    ui(new Ui::QGCUASFileView)
{
    ui->setupUi(this);

    connect(ui->testButton, SIGNAL(clicked()), _manager, SLOT(nothingMessage()));
    connect(ui->listFilesButton, SIGNAL(clicked()), this, SLOT(listFiles()));
    connect(ui->downloadButton, SIGNAL(clicked()), this, SLOT(downloadFiles()));

    connect(_manager, SIGNAL(statusMessage(QString)), ui->messageArea, SLOT(appendPlainText(QString)));
    connect(_manager, SIGNAL(errorMessage(QString)), ui->messageArea, SLOT(appendPlainText(QString)));
    connect(_manager, SIGNAL(resetStatusMessages()), ui->messageArea, SLOT(clear()));
}

QGCUASFileView::~QGCUASFileView()
{
    delete ui;
}

void QGCUASFileView::listFiles()
{
    _manager->listDirectory(ui->pathLineEdit->text());
}

void QGCUASFileView::downloadFiles()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Download Directory"),
                                                     QDir::homePath(),
                                                     QFileDialog::ShowDirsOnly
                                                     | QFileDialog::DontResolveSymlinks);
    // And now download to this location
    _manager->downloadPath(ui->pathLineEdit->text(), QDir(dir));
}
