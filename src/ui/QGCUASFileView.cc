#include "QGCUASFileView.h"
#include "uas/QGCUASFileManager.h"
#include "ui_QGCUASFileView.h"

QGCUASFileView::QGCUASFileView(QWidget *parent, QGCUASFileManager *manager) :
    QWidget(parent),
    _manager(manager),
    ui(new Ui::QGCUASFileView)
{
    ui->setupUi(this);

    connect(ui->testButton, SIGNAL(clicked()), _manager, SLOT(nothingMessage()));
}

QGCUASFileView::~QGCUASFileView()
{
    delete ui;
}
