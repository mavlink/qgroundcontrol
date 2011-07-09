#include "QGCMapToolBar.h"
#include "QGCMapWidget.h"
#include "ui_QGCMapToolBar.h"

QGCMapToolBar::QGCMapToolBar(QWidget *parent) :
    QWidget(parent),
    map(NULL),
    ui(new Ui::QGCMapToolBar)
{
    ui->setupUi(this);
}

void QGCMapToolBar::setMap(QGCMapWidget* map)
{
    this->map = map;

    if (map)
    {
        connect(ui->goToButton, SIGNAL(clicked()), map, SLOT(showGoToDialog()));
        connect(ui->goHomeButton, SIGNAL(clicked()), map, SLOT(goHome()));
    }
}

QGCMapToolBar::~QGCMapToolBar()
{
    delete ui;
}
