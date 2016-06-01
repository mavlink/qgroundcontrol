/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief Implementation of MAVLinkSettingsWidget
 *   @author Lorenz Meier <mail@qgroundcontrol.org>
 */

#include <QFileInfo>
#include <QStandardPaths>

#include "MAVLinkSettingsWidget.h"
#include "LinkManager.h"
#include "UDPLink.h"
#include "QGCApplication.h"
#include "ui_MAVLinkSettingsWidget.h"
#include <QSettings>

MAVLinkSettingsWidget::MAVLinkSettingsWidget(MAVLinkProtocol* protocol, QWidget *parent) :
    QWidget(parent),
    protocol(protocol),
    m_ui(new Ui::MAVLinkSettingsWidget)
{
    m_ui->setupUi(this);

    m_ui->gridLayout->setAlignment(Qt::AlignTop);

    // AUTH
    m_ui->droneOSCheckBox->setChecked(protocol->getAuthEnabled());
    QSettings settings;
    m_ui->droneOSComboBox->setCurrentIndex(m_ui->droneOSComboBox->findText(settings.value("DRONELINK_HOST", "dronelink.io:14555").toString()));
    m_ui->droneOSLineEdit->setText(protocol->getAuthKey());

    // Initialize state
    m_ui->versionCheckBox->setChecked(protocol->versionCheckEnabled());
    m_ui->multiplexingCheckBox->setChecked(protocol->multiplexingEnabled());
    m_ui->systemIdSpinBox->setValue(protocol->getSystemId());

    m_ui->paramGuardCheckBox->setChecked(protocol->paramGuardEnabled());
    m_ui->paramRetransmissionSpinBox->setValue(protocol->getParamRetransmissionTimeout());
    m_ui->paramRewriteSpinBox->setValue(protocol->getParamRewriteTimeout());

    m_ui->actionGuardCheckBox->setChecked(protocol->actionGuardEnabled());
    m_ui->actionRetransmissionSpinBox->setValue(protocol->getActionRetransmissionTimeout());

    // Version check
    connect(protocol, &MAVLinkProtocol::versionCheckChanged, m_ui->versionCheckBox, &QCheckBox::setChecked);
    connect(m_ui->versionCheckBox, &QCheckBox::toggled, protocol, &MAVLinkProtocol::enableVersionCheck);
    // System ID
    connect(protocol, &MAVLinkProtocol::systemIdChanged, m_ui->systemIdSpinBox, &QSpinBox::setValue);
    connect(m_ui->systemIdSpinBox,static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), protocol, &MAVLinkProtocol::setSystemId);
    // Multiplexing
    connect(protocol, &MAVLinkProtocol::multiplexingChanged, m_ui->multiplexingCheckBox, &QCheckBox::setChecked);
    connect(m_ui->multiplexingCheckBox, &QCheckBox::toggled, protocol, &MAVLinkProtocol::enableMultiplexing);
    // Parameter guard
    connect(protocol, &MAVLinkProtocol::paramGuardChanged, m_ui->paramGuardCheckBox, &QCheckBox::setChecked);
    connect(m_ui->paramGuardCheckBox, &QCheckBox::toggled, protocol, &MAVLinkProtocol::enableParamGuard);
    connect(protocol, &MAVLinkProtocol::paramRetransmissionTimeoutChanged, m_ui->paramRetransmissionSpinBox, &QSpinBox::setValue);
    connect(m_ui->paramRetransmissionSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), protocol, &MAVLinkProtocol::setParamRetransmissionTimeout);
    connect(protocol, &MAVLinkProtocol::paramRewriteTimeoutChanged, m_ui->paramRewriteSpinBox, &QSpinBox::setValue);
    connect(m_ui->paramRewriteSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), protocol, &MAVLinkProtocol::setParamRewriteTimeout);
    // Action guard
    connect(protocol, &MAVLinkProtocol::actionGuardChanged, m_ui->actionGuardCheckBox, &QCheckBox::setChecked);
    connect(m_ui->actionGuardCheckBox, &QCheckBox::toggled, protocol, &MAVLinkProtocol::enableActionGuard);
    connect(protocol, &MAVLinkProtocol::actionRetransmissionTimeoutChanged, m_ui->actionRetransmissionSpinBox, &QSpinBox::setValue);
    connect(m_ui->actionRetransmissionSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), protocol, &MAVLinkProtocol::setActionRetransmissionTimeout);
    // MAVLink AUTH
    connect(protocol, &MAVLinkProtocol::authChanged, m_ui->droneOSCheckBox, &QCheckBox::setChecked);
    connect(m_ui->droneOSCheckBox, &QCheckBox::toggled, this, &MAVLinkSettingsWidget::enableDroneOS);
    connect(protocol, &MAVLinkProtocol::authKeyChanged, m_ui->droneOSLineEdit, &QLineEdit::setText);
    connect(m_ui->droneOSLineEdit, &QLineEdit::textChanged, this, &MAVLinkSettingsWidget::setDroneOSKey);

    // Drone OS
    connect(m_ui->droneOSComboBox, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentIndexChanged), this, &MAVLinkSettingsWidget::setDroneOSHost);
    // FIXME Manually trigger this action here, this brings control code to UI = BAD!
    setDroneOSHost(m_ui->droneOSComboBox->currentText());

    // Update values
    m_ui->versionLabel->setText(tr("MAVLINK_VERSION: %1").arg(protocol->getVersion()));

    // Connect visibility updates
    connect(protocol, &MAVLinkProtocol::versionCheckChanged, m_ui->versionLabel, &QWidget::setVisible);
    m_ui->versionLabel->setVisible(protocol->versionCheckEnabled());

//    // Multiplexing visibility
//    connect(protocol, SIGNAL(multiplexingChanged(bool)), m_ui->multiplexingFilterCheckBox, SLOT(setVisible(bool)));
//    m_ui->multiplexingFilterCheckBox->setVisible(protocol->multiplexingEnabled());
//    connect(protocol, SIGNAL(multiplexingChanged(bool)), m_ui->multiplexingFilterLineEdit, SLOT(setVisible(bool)));
//    m_ui->multiplexingFilterLineEdit->setVisible(protocol->multiplexingEnabled());

    // Param guard visibility
    connect(protocol, &MAVLinkProtocol::paramGuardChanged, m_ui->paramRetransmissionSpinBox, &QWidget::setVisible);
    m_ui->paramRetransmissionSpinBox->setVisible(protocol->paramGuardEnabled());
    connect(protocol, &MAVLinkProtocol::paramGuardChanged, m_ui->paramRetransmissionLabel, &QWidget::setVisible);
    m_ui->paramRetransmissionLabel->setVisible(protocol->paramGuardEnabled());
    connect(protocol, &MAVLinkProtocol::paramGuardChanged, m_ui->paramRewriteSpinBox, &QWidget::setVisible);
    m_ui->paramRewriteSpinBox->setVisible(protocol->paramGuardEnabled());
    connect(protocol, &MAVLinkProtocol::paramGuardChanged, m_ui->paramRewriteLabel, &QWidget::setVisible);
    m_ui->paramRewriteLabel->setVisible(protocol->paramGuardEnabled());
    // Action guard visibility
    connect(protocol, &MAVLinkProtocol::actionGuardChanged, m_ui->actionRetransmissionSpinBox, &QWidget::setVisible);
    m_ui->actionRetransmissionSpinBox->setVisible(protocol->actionGuardEnabled());
    connect(protocol, &MAVLinkProtocol::actionGuardChanged, m_ui->actionRetransmissionLabel, &QWidget::setVisible);
    m_ui->actionRetransmissionLabel->setVisible(protocol->actionGuardEnabled());

    // TODO implement filtering
    // and then remove these two lines
    m_ui->multiplexingFilterCheckBox->setVisible(false);
    m_ui->multiplexingFilterLineEdit->setVisible(false);
}

void MAVLinkSettingsWidget::enableDroneOS(bool enable)
{
    // Enable multiplexing
    protocol->enableMultiplexing(enable);
    // Get current selected host and port
    QString hostString = m_ui->droneOSComboBox->currentText();
    //QString host = hostString.split(":").first();

    LinkManager*    linkMgr = qgcApp()->toolbox()->linkManager();
    UDPLink*        firstUdp = NULL;

    // Delete from all lists first
    for (int i=0; i<linkMgr->links()->count(); i++) {
        LinkInterface*  link = linkMgr->links()->value<LinkInterface*>(i);
        UDPLink*        udp = qobject_cast<UDPLink*>(link);

        if (udp)
        {
            if (!firstUdp) firstUdp = udp;
            // Remove current hosts
            for (int i = 0; i < m_ui->droneOSComboBox->count(); ++i)
            {
                QString oldHostString = m_ui->droneOSComboBox->itemText(i);
                oldHostString = hostString.split(":").first();
                udp->removeHost(oldHostString);
            }
        }
    }

    // Re-add if enabled
    if (enable)
    {
        if (firstUdp)
        {
            firstUdp->addHost(hostString);
        }
        // Set key
        protocol->setAuthKey(m_ui->droneOSLineEdit->text().trimmed());
        QSettings settings;
        settings.setValue("DRONELINK_HOST", m_ui->droneOSComboBox->currentText());
    }
    protocol->enableAuth(enable);
}

void MAVLinkSettingsWidget::setDroneOSKey(QString key)
{
    Q_UNUSED(key);
    enableDroneOS(m_ui->droneOSCheckBox->isChecked());
}

void MAVLinkSettingsWidget::setDroneOSHost(QString host)
{
    Q_UNUSED(host);
    enableDroneOS(m_ui->droneOSCheckBox->isChecked());
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

void MAVLinkSettingsWidget::hideEvent(QHideEvent* event)
{
    Q_UNUSED(event);
    protocol->storeSettings();
}
