#ifndef QGCTCPLINKCONFIGURATION_H
#define QGCTCPLINKCONFIGURATION_H

#include <QWidget>

#include "TCPLink.h"

namespace Ui
{
class QGCTCPLinkConfiguration;
}

class QGCTCPLinkConfiguration : public QWidget
{
    Q_OBJECT

public:
    explicit QGCTCPLinkConfiguration(TCPLink* link, QWidget *parent = 0);
    ~QGCTCPLinkConfiguration();

public slots:

protected:
    void changeEvent(QEvent *e);

    TCPLink* link;    ///< TCP link instance this widget configures

private:
    Ui::QGCTCPLinkConfiguration *ui;
};

#endif // QGCTCPLINKCONFIGURATION_H
