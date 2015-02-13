#include <QPushButton>

#include "SerialLink.h"
#include "SerialConfigurationWindow.h"
#include "QGCUDPLinkConfiguration.h"
#include "QGCCommConfiguration.h"
#include "ui_QGCCommConfiguration.h"

QGCCommConfiguration::QGCCommConfiguration(QWidget *parent, LinkConfiguration *config) :
    QDialog(parent),
    _ui(new Ui::QGCCommConfiguration)
{
    _ui->setupUi(this);
    // Add link types
    _config = config;
    _ui->typeCombo->addItem(tr("Select Type"),  LinkConfiguration::TypeLast);
    _ui->typeCombo->addItem(tr("Serial"),       LinkConfiguration::TypeSerial);
    _ui->typeCombo->addItem(tr("UDP"),          LinkConfiguration::TypeUdp);
#ifdef UNITTEST_BUILD
    _ui->typeCombo->addItem(tr("Mock"),         LinkConfiguration::TypeMock);
#endif

#if 0
    _ui->typeCombo->addItem(tr("TCP"),          LinkConfiguration::TypeTcp);

#ifdef QGC_RTLAB_ENABLED
    _ui->typeCombo->addItem(tr("Opal-RT Link"), LinkConfiguration::TypeOpal);
#endif
#ifdef QGC_XBEE_ENABLED
    _ui->typeCombo->addItem(tr("Xbee API"),     LinkConfiguration::TypeXbee);
#endif
#endif

    _ui->typeCombo->setEditable(false);
    if(config && !config->name().isEmpty()) {
        _ui->nameEdit->setText(config->name());
    } else {
        _ui->nameEdit->setText(tr("Unnamed"));
    }
    if(!config) {
        setWindowTitle(tr("Add New Communication Link"));
    } else {
        setWindowTitle(tr("Edit Communication Link"));
        _loadTypeConfigWidget(config->type());
        _ui->typeCombo->setEnabled(false);
    }
    _updateUI();
}

QGCCommConfiguration::~QGCCommConfiguration()
{
    delete _ui;
}

void QGCCommConfiguration::on_typeCombo_currentIndexChanged(int index)
{
    int type = _ui->typeCombo->itemData(index).toInt();
    _changeLinkType(type);
}

void QGCCommConfiguration::_changeLinkType(int type)
{
    //-- Do we need to change anything?
    if(type == LinkConfiguration::TypeLast || (_config && _config->type() == type)) {
        return;
    }
    // Switching connection type. Delete old config.
    delete _config;
    // Create new config instance
    QString name = _ui->nameEdit->text();
    if(name.isEmpty()) {
        name = tr("Untitled");
        _ui->nameEdit->setText(name);
    }
    _config = LinkConfiguration::createSettings(type, name);
    Q_ASSERT(_config != NULL);
    _loadTypeConfigWidget(type);
    _updateUI();
}

void QGCCommConfiguration::_loadTypeConfigWidget(int type)
{
    Q_ASSERT(_config != NULL);
    switch(type) {
        case LinkConfiguration::TypeSerial: {
            QWidget* conf = new SerialConfigurationWindow((SerialConfiguration*)_config, this);
            _ui->linkScrollArea->setWidget(conf);
            _ui->linkGroupBox->setTitle(tr("Serial Link"));
            _ui->typeCombo->setCurrentIndex(_ui->typeCombo->findData(LinkConfiguration::TypeSerial));
        }
        break;
        case LinkConfiguration::TypeUdp: {
            QWidget* conf = new QGCUDPLinkConfiguration((UDPConfiguration*)_config, this);
            _ui->linkScrollArea->setWidget(conf);
            _ui->linkGroupBox->setTitle(tr("UDP Link"));
            _ui->typeCombo->setCurrentIndex(_ui->typeCombo->findData(LinkConfiguration::TypeUdp));
        }
        break;
#ifdef UNITTEST_BUILD
        case LinkConfiguration::TypeMock: {
            _ui->linkScrollArea->setWidget(NULL);
            _ui->linkGroupBox->setTitle(tr("Mock Link"));
            _ui->typeCombo->setCurrentIndex(_ui->typeCombo->findData(LinkConfiguration::TypeMock));
        }
        break;
#endif
        // Cannot be the case, but in case it gets here, we cannot continue.
        default:
            reject();
            break;
    }
    // Remove "Select Type" once something is selected
    int idx = _ui->typeCombo->findData(LinkConfiguration::TypeLast);
    if(idx >= 0) {
        _ui->typeCombo->removeItem(idx);
    }
}

void QGCCommConfiguration::_updateUI()
{
    bool enableOK = false;
    if(_config) {
        if(!_ui->nameEdit->text().isEmpty()) {
            enableOK = true;
        }
    }
    QPushButton* ok = _ui->buttonBox->button(QDialogButtonBox::Ok);
    Q_ASSERT(ok != NULL);
    ok->setEnabled(enableOK);
}

void QGCCommConfiguration::on_buttonBox_accepted()
{
    if(_config) {
        _config->setName(_ui->nameEdit->text());
    }
    accept();
}

void QGCCommConfiguration::on_buttonBox_rejected()
{
    reject();
}

void QGCCommConfiguration::on_nameEdit_textEdited(const QString &arg1)
{
    Q_UNUSED(arg1);
    _updateUI();
}
