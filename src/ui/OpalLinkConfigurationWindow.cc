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
    connect(link, SIGNAL(connected(bool)), this, SLOT(allowSettingsAccess(bool)));
    this->show();
}

void OpalLinkConfigurationWindow::allowSettingsAccess(bool enabled)
{
    ui.paramFileButton->setEnabled(enabled);
    ui.servoConfigFileButton->setEnabled(enabled);
}
