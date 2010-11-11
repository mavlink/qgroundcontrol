#include "slugshilsim.h"
#include "ui_slugshilsim.h"
#include "LinkManager.h"

SlugsHilSim::SlugsHilSim(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SlugsHilSim)
{
    ui->setupUi(this);
    linkAdded();

}

SlugsHilSim::~SlugsHilSim()
{
    delete ui;
}

void SlugsHilSim::linkAdded(void){

  ui->cb_mavlinkLinks->clear();

  QList<LinkInterface *> linkList = LinkManager::instance()->getLinks() ;

  for (int i = 0; i< linkList.size(); i++){
    ui->cb_mavlinkLinks->addItem((linkList.takeFirst())->getName());
  }

}
