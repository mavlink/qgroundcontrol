#include "OsdConfig.h"
#include <QMessageBox>

OsdConfig::OsdConfig(QWidget *parent) : AP2ConfigWidget(parent)
{
    ui.setupUi(this);
    connect(ui.enablePushButton,SIGNAL(clicked()),this,SLOT(enableButtonClicked()));
    initConnections();

}

OsdConfig::~OsdConfig()
{
}
void OsdConfig::enableButtonClicked()
{
    if (!m_uas)
    {
        showNullMAVErrorMessageBox();
        return;
    }
    m_uas->getParamManager()->setParameter(1,"SR0_EXT_STAT",2);
    m_uas->getParamManager()->setParameter(1,"SR0_EXTRA1",10);
    m_uas->getParamManager()->setParameter(1,"SR0_EXTRA2",10);
    m_uas->getParamManager()->setParameter(1,"SR0_EXTRA3",2);
    m_uas->getParamManager()->setParameter(1,"SR0_POSITION",3);
    m_uas->getParamManager()->setParameter(1,"SR0_RAW_CTRL",2);
    m_uas->getParamManager()->setParameter(1,"SR0_RAW_SENS",2);
    m_uas->getParamManager()->setParameter(1,"SR0_RC_CHAN",2);
}
