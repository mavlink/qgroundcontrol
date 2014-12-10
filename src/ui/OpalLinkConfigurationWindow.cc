#include "OpalLinkConfigurationWindow.h"

OpalLinkConfigurationWindow::OpalLinkConfigurationWindow(OpalLink* link,
        QWidget *parent,
        Qt::WindowFlags flags) :
    QWidget(parent, flags),
    link(link)

{


    ui.setupUi(this);

    ui.opalInstIDSpinBox->setValue(this->link->getOpalInstID());

    connect(ui.opalInstIDSpinBox, SIGNAL(valueChanged(int)), link, SLOT(setOpalInstID(int)));
    connect(link, &LinkInterface::connected, this, OpalLinkConfigurationWindow::_linkConnected);
    connect(link, &LinkInterface::disconnected, this, OpalLinkConfigurationWindow::_linkDisConnected);
    this->show();
}

void OpalLinkConfigurationWindow::_linkConnected(void)
{
    _allowSettingsAccess(true);
}

void OpalLinkConfigurationWindow::_linkConnected(void)
{
    _allowSettingsAccess(false);
}

void OpalLinkConfigurationWindow::_allowSettingsAccess(bool enabled)
{
    ui.paramFileButton->setEnabled(enabled);
    ui.servoConfigFileButton->setEnabled(enabled);
}
