#include "MAVLinkSettingsWidget.h"
#include "ui_MAVLinkSettingsWidget.h"

MAVLinkSettingsWidget::MAVLinkSettingsWidget(MAVLinkProtocol* protocol, QWidget *parent) :
    QWidget(parent),
    protocol(protocol),
    m_ui(new Ui::MAVLinkSettingsWidget)
{
    m_ui->setupUi(this);

    // Connect actions
    connect(protocol, SIGNAL(heartbeatChanged(bool)), m_ui->heartbeatCheckBox, SLOT(setChecked(bool)));
    connect(m_ui->heartbeatCheckBox, SIGNAL(toggled(bool)), protocol, SLOT(enableHeartbeats(bool)));
    connect(protocol, SIGNAL(loggingChanged(bool)), m_ui->loggingCheckBox, SLOT(setChecked(bool)));
    connect(m_ui->loggingCheckBox, SIGNAL(toggled(bool)), protocol, SLOT(enableLogging(bool)));

    // Initialize state
    m_ui->heartbeatCheckBox->setChecked(protocol->heartbeatsEnabled());
    m_ui->loggingCheckBox->setChecked(protocol->loggingEnabled());
}

MAVLinkSettingsWidget::~MAVLinkSettingsWidget()
{
    delete m_ui;
}

void MAVLinkSettingsWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
