#ifndef QGCCOMMCONFIGURATION_H
#define QGCCOMMCONFIGURATION_H

#include <QWidget>
#include <QDialog>

#include "LinkConfiguration.h"

namespace Ui {
class QGCCommConfiguration;
}

class QGCCommConfiguration : public QDialog
{
    Q_OBJECT

public:
    explicit QGCCommConfiguration(QWidget *parent, LinkConfiguration* config = 0);
    ~QGCCommConfiguration();

    enum {
        QGC_LINK_SERIAL,
        QGC_LINK_UDP,
        QGC_LINK_TCP,
        QGC_LINK_SIMULATION,
        QGC_LINK_FORWARDING,
#ifdef UNITTEST_BUILD
        QGC_LINK_MOCK,
#endif
#ifdef  QGC_XBEE_ENABLED
        QGC_LINK_XBEE,
#endif
#ifdef  QGC_RTLAB_ENABLED
        QGC_LINK_OPAL
#endif
    };

    LinkConfiguration* getConfig() { return _config; }

private slots:
    void on_typeCombo_currentIndexChanged(int index);
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();
    void on_nameEdit_textEdited(const QString &arg1);

private:
    void _changeLinkType(int type);
    void _loadTypeConfigWidget(int type);
    void _updateUI();

    Ui::QGCCommConfiguration* _ui;
    LinkConfiguration*        _config;
};

#endif // QGCCOMMCONFIGURATION_H
